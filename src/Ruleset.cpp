#include "ll/Ruleset.hpp"
#include "ll/ActionType.hpp"
#include "ll/config.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <vector>

extern "C" {
#include <linux/landlock.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
}

namespace landlock
{
Ruleset::Ruleset(
	// NOLINTNEXTLINE(*-easily-swappable-parameters)
	const ActionVec& handled_access_fs,
	const ActionVec& handled_access_net
)
{
	if (handled_access_fs.empty() && handled_access_net.empty()) {
		throw std::invalid_argument{
			"Landlock without handled access is not allowed"
		};
	}

	if (not read_abi_version()) {
		return;
	}

	init_ruleset(handled_access_fs, handled_access_net);
}

Ruleset::~Ruleset()
{
	if (ruleset_fd_ > 0) {
		::close(ruleset_fd_);
	}
}

void Ruleset::enforce(bool set_no_new_privs) const
{
	if (set_no_new_privs) {
		const int res =
			::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0
			); // NOLINT(*-vararg)
		assert_res(res);
	}

	if (landlock_enabled()) {
		const int res = landlock_restrict_self(ruleset_fd_);
		assert_res(res);
	}
}

bool Ruleset::read_abi_version()
{
	const int res = landlock_create_ruleset(
		nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION
	);
	if (res == -1 and errno == ENOSYS) {
		return false;
	}
	assert_res(res);

	abi_version_ = res;
	return true;
}

void Ruleset::init_ruleset(
	// NOLINTNEXTLINE(*-easily-swappable-parameters)
	const ActionVec& handled_access_fs,
	[[maybe_unused]] const ActionVec& handled_access_net
)
{
	const landlock_ruleset_attr attr
	{
		ActionType::join(abi_version_, handled_access_fs).type_code()
#if LLPP_BUILD_LANDLOCK_API >= 4
			,
			ActionType::join(abi_version_, handled_access_net)
				.type_code()
#endif
	};

	const int res = landlock_create_ruleset(&attr, sizeof(attr), 0);
	assert_res(res);

	ruleset_fd_ = res;
}

// NOLINTBEGIN(*-vararg)
int Ruleset::landlock_create_ruleset(
	const landlock_ruleset_attr* attr, std::size_t size, std::uint32_t flags
)
{
	return static_cast<int>(
		::syscall(SYS_landlock_create_ruleset, attr, size, flags)
	);
}

int Ruleset::landlock_add_rule(
	int ruleset_fd,
	landlock_rule_type rule_type,
	const void* rule_attr,
	std::uint32_t flags
)
{
	return static_cast<int>(::syscall(
		SYS_landlock_add_rule, ruleset_fd, rule_type, rule_attr, flags
	));
}

int Ruleset::landlock_restrict_self(int ruleset_fd, std::uint32_t flags)
{
	return static_cast<int>(
		::syscall(SYS_landlock_restrict_self, ruleset_fd, flags)
	);
}
// NOLINTEND(*-vararg)

void Ruleset::assert_res(int res)
{
	if (res < 0) {
		throw std::system_error{
			std::error_code{errno, std::system_category()}
		};
	}
}
} // namespace landlock
