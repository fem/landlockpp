#include "ll/Ruleset.hpp"
#include "ll/ActionType.hpp"
#include "ll/Rule.hpp"
#include "ll/Scope.hpp"
#include "ll/config.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "test.hpp"

using landlock::Ruleset;

TEST_CASE("Ruleset::default init fails")
{
	std::unique_ptr<Ruleset> ruleset;
	REQUIRE_THROWS_AS(
		ruleset = std::make_unique<Ruleset>(), std::invalid_argument
	);
}

// NOLINTBEGIN(*-vararg)
TEST_CASE("Ruleset::rules")
{
	const std::filesystem::path allowed_test_path{"/proc"};
	const std::filesystem::path disallowed_test_path{"/usr/bin"};
	Ruleset ruleset{
		{landlock::action::FS_READ_FILE,
		 landlock::action::FS_READ_DIR,
		 landlock::action::FS_WRITE_FILE,
		 landlock::action::FS_TRUNCATE,
		 landlock::action::FS_EXECUTE}
	};

	{
		// Provoke that the rules might get deleted before enforce()
		landlock::PathBeneathRule rule1;
		rule1.add_path(allowed_test_path)
			.add_action(landlock::action::FS_READ_FILE)
			.add_action(landlock::action::FS_READ_DIR)
			.add_action(landlock::action::FS_WRITE_FILE)
			.add_action(landlock::action::FS_TRUNCATE);
		landlock::PathBeneathRule rule2;
		rule2.add_path(disallowed_test_path)
			.add_action(landlock::action::FS_READ_DIR);
		ruleset.add_rule(std::move(rule1)).add_rule(std::move(rule2));
	}
	ruleset.enforce(true);

	const int allowed_fd =
		::open((allowed_test_path / "meminfo").c_str(), O_RDONLY);
	CHECK(allowed_fd >= 0);
	if (allowed_fd < 0) {
		// NOLINTNEXTLINE(*-mt-unsafe)
		std::clog << "errno: " << errno << " (" << strerror(errno)
			  << ")\n";
	}
	if (allowed_fd > 0) {
		::close(allowed_fd);
	}

	if (ruleset.landlock_enabled()) {
		const int disallowed_fd =
			::open((disallowed_test_path / "env").c_str(),
			       O_RDONLY);
		REQUIRE(disallowed_fd < 0);
		CHECK(errno == EACCES);
		if (disallowed_fd > 0) {
			::close(disallowed_fd);
		}
	}
}

TEST_CASE("Ruleset::IPC scope")
{
	// NOLINTBEGIN(*-mt-unsafe, *-magic-numbers)
	using namespace std::chrono_literals;

	int res;
	sigset_t sigs;
	sigset_t original_sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGUSR1);

	const pid_t pid = getpid();

	int sigfd = -1;
	int efd = -1;
	int epfd = -1;

	bool expect_eperm = false;

	std::unique_ptr<std::thread> signaller;
	std::packaged_task<bool()> signaling_task{[&]() -> bool {
		landlock::Ruleset ruleset{{}, {}, {landlock::scope::SIGNAL}};
		ruleset.enforce();
		// No scope support before ABI 6
		expect_eperm = ruleset.effective_abi_version() >= 6;
		if (ruleset.effective_abi_version() < 6) {
			WARN("Effective ABI version too low. This test won't "
			     "work as expected");
		}

		const int kill_res = kill(pid, SIGUSR1);
		std::uint64_t buf = 1;
		const auto err = errno;

		if (kill_res >= 0) {
			const ssize_t write_res =
				::write(efd, &buf, sizeof(buf));
			CHECK(write_res == sizeof(buf));
			return false;
		}

		if (err != EPERM) {
			const ssize_t write_res =
				::write(efd, &buf, sizeof(buf));
			CHECK(write_res == sizeof(buf));
			throw std::system_error{err, std::system_category()};
		}

		const ssize_t write_res = ::write(efd, &buf, sizeof(buf));
		CHECK(write_res == sizeof(buf));
		return true;
	}};

	auto errcode = [&](const std::string& pfx,
			   std::decay_t<decltype(errno)> err) {
		const std::string msg =
			err > 0 ? strerror(err) : "(no errno message)";
		if (signaller) {
			if (signaller->joinable()) {
				signaller->join();
			}

			signaller.reset();
		}
		if (epfd >= 0) {
			::close(epfd);
		}
		if (efd >= 0) {
			::close(efd);
		}
		if (sigfd >= 0) {
			::close(sigfd);
		}
		sigprocmask(SIG_SETMASK, &original_sigs, &sigs);
		FAIL(pfx + " failed: " + msg);
	};

	res = sigprocmask(SIG_BLOCK, &sigs, &original_sigs);
	if (res != 0) {
		FAIL(strerror(errno));
	}

	sigfd = signalfd(-1, &sigs, SFD_NONBLOCK);
	if (sigfd < 0) {
		errcode("signalfd()", errno);
	}

	efd = eventfd(0, EFD_NONBLOCK);
	if (efd < 0) {
		errcode("eventfd()", errno);
	}

	epfd = epoll_create1(0);
	if (epfd < 0) {
		errcode("epoll_create1()", errno);
	}

	epoll_event epe_sig{};
	epe_sig.events = EPOLLIN;
	epe_sig.data.fd = sigfd;
	epoll_event epe_efd{};
	epe_efd.events = EPOLLIN;
	epe_efd.data.fd = efd;

	res = epoll_ctl(epfd, EPOLL_CTL_ADD, sigfd, &epe_sig);
	if (res < 0) {
		errcode("epoll_ctl epe_sig", errno);
	}
	res = epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &epe_efd);
	if (res < 0) {
		errcode("epoll_ctl epe_efd", errno);
	}

	signaller = std::make_unique<std::thread>([&]() { signaling_task(); });

	epoll_event read_event{};
	CHECK(signaling_task.valid());
	std::future signal_future = signaling_task.get_future();
	CHECK(signal_future.valid());
	while ((res = epoll_wait(epfd, &read_event, 1, 1000)) > 0) {
		try {
			const bool signal_eperm = signal_future.get();
			if (expect_eperm) {
				CHECK(signal_eperm);
				CHECK(read_event.data.fd == efd);
			} else {
				CHECK_FALSE(signal_eperm);
				CHECK(read_event.data.fd == sigfd);
			}

			std::uint64_t buf;
			while (::read(read_event.data.fd, &buf, sizeof(buf)) > 0
			) {
				// nothing
			}
		} catch (std::system_error& e) {
			errcode("exception in signaling task",
				e.code().value());
		}
		break;
	}
	if (res == 0) {
		errcode("epoll: timeout;", 0);
	}
	// NOLINTEND(*-vararg)

	if (signaller->joinable()) {
		signaller->join();
	}

	::close(epfd);
	::close(efd);
	::close(sigfd);
	sigprocmask(SIG_BLOCK, &original_sigs, &sigs);
	// NOLINTEND(*-mt-unsafe, *-magic-numbers)
}
// NOLINTEND(*-vararg)
