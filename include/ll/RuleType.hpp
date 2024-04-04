#pragma once

#include <type_traits>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/config.h>

namespace landlock
{
constexpr std::underlying_type_t<landlock_rule_type> INVALID_RULE_TYPE = 0;

template <typename T>
struct RuleType {
	constexpr static landlock_rule_type TYPE_CODE =
		static_cast<landlock_rule_type>(INVALID_RULE_TYPE);
};

template <>
struct RuleType<landlock_path_beneath_attr> {
	constexpr static landlock_rule_type TYPE_CODE =
		LANDLOCK_RULE_PATH_BENEATH;
};

#if LLPP_BUILD_LANDLOCK_ABI >= 4
template <>
struct RuleType<landlock_net_port_attr> {
	constexpr static landlock_rule_type TYPE_CODE = LANDLOCK_RULE_NET_PORT;
};
#else
#endif
} // namespace landlock
