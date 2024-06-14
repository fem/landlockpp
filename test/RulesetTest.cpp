#include "ll/Ruleset.hpp"
#include "ll/ActionType.hpp"
#include "ll/Rule.hpp"
#include "ll/config.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <fcntl.h>
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
// NOLINTEND(*-vararg)
