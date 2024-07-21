#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <type_traits>
#include <vector>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/config.h>
#include <ll/typing.hpp>

namespace landlock
{
/**
 * Compatible rules for action types
 */
enum class ActionRuleType : std::underlying_type_t<landlock_rule_type> {
	PATH_BENEATH = LANDLOCK_RULE_PATH_BENEATH,
#if LLPP_BUILD_LANDLOCK_API >= 4
	NET_PORT = LANDLOCK_RULE_NET_PORT,
#else
	NET_PORT = 0,
#endif
};

/**
 * Landlock action type
 *
 * This represents a type of action that can be secured by landlock. Each action
 * is associated with a type code and a minimum ABI version required to secure
 * this action using landlock.
 */
template <typename SuppT>
class ActionType
{
public:
	// NOLINTNEXTLINE(*-easily-swappable-parameters)
	constexpr ActionType(std::uint64_t type_code, int min_abi) :
		type_code_(type_code), min_abi_(min_abi)
	{
	}

	constexpr ActionType(const ActionType&) = default;
	constexpr ActionType& operator=(const ActionType&) = default;
	constexpr ActionType(ActionType&&) = default;
	constexpr ActionType& operator=(ActionType&&) = default;
	~ActionType() = default;

	/**
	 * Combine two action types
	 *
	 * The combination is the bitwise OR of the type codes (each action is
	 * represented by one bit in the Linux headers) and the minimum ABI
	 * version of the resulting action is raised to the maximum of the two
	 * actions.
	 */
	constexpr ActionType& operator|=(const ActionType& other) noexcept
	{
		type_code_ |= other.type_code_;
		min_abi_ = std::max(min_abi_, other.min_abi_);
		return *this;
	}

	[[nodiscard]] constexpr std::uint64_t type_code() const noexcept
	{
		return type_code_;
	}
	[[nodiscard]] constexpr int min_abi() const noexcept
	{
		return min_abi_;
	}

private:
	std::uint64_t type_code_;
	int min_abi_;
};

namespace typing
{
template <typename T, T... supp>
struct Unwrap<ActionType<ValWrapper<T, supp...>>> {
	using type = ValWrapper<T, supp...>;
};
} // namespace typing

template <typename RuleT, RuleT reduce_to, RuleT... supp>
constexpr const ActionType<typing::ValWrapper<RuleT, reduce_to>>&
reduce(const ActionType<typing::ValWrapper<RuleT, supp...>>& action)
{
	static_assert(
		typing::is_element<RuleT, reduce_to, supp...>(),
		"Trying to reduce to unsupported type"
	);
	// TODO can we get rid of reinterpret_cast?
	return *reinterpret_cast<
		const ActionType<typing::ValWrapper<RuleT, reduce_to>>*>(&action
	);
}
} // namespace landlock

template <typename RuleT, RuleT... supp_lhs, RuleT... supp_rhs>
constexpr landlock::ActionType<landlock::typing::UnionT<
	RuleT,
	landlock::typing::ValWrapper<RuleT, supp_lhs...>,
	landlock::typing::ValWrapper<RuleT, supp_rhs...>>>
operator|(
	const landlock::ActionType<
		landlock::typing::ValWrapper<RuleT, supp_lhs...>>& lhs,
	const landlock::ActionType<
		landlock::typing::ValWrapper<RuleT, supp_rhs...>>& rhs
) noexcept
{
	return {lhs.type_code() | rhs.type_code(),
		std::max(lhs.min_abi(), rhs.min_abi())};
}

template <typename T, typename U>
constexpr bool operator==(
	[[maybe_unused]] const landlock::ActionType<T>& lhs,
	[[maybe_unused]] const landlock::ActionType<U>& rhs
) noexcept
{
	// Actual equality is checked in template specializations
	return false;
}

template <typename supported>
constexpr bool operator==(
	const landlock::ActionType<supported>& lhs,
	const landlock::ActionType<supported>& rhs
) noexcept
{
	return lhs.type_code() == rhs.type_code() &&
	       lhs.min_abi() == rhs.min_abi();
}

template <typename T, typename U>
constexpr bool operator!=(
	const landlock::ActionType<T>& lhs, const landlock::ActionType<U>& rhs
) noexcept
{
	return not(lhs == rhs);
}

template <typename supported>
std::ostream&
operator<<(std::ostream& out, const landlock::ActionType<supported>& atype)
{
	const auto fmt = out.flags();
	out << std::hex << std::setfill('0')
	    << std::setw(sizeof(std::uint64_t) * 2) << atype.type_code() << '/'
	    << std::dec << atype.min_abi();
	out.flags(fmt);
	return out;
}

namespace landlock
{
/**
 * Predefined types for actions
 *
 * These action types are predefined with names similar to the ones from
 * linux/landlock.h. If any of these is not available in the active landlock ABI
 * at compile time, they are replaced with an invalid (no-op) action for
 * backwards compatibility with older Kernel and header versions.
 */
namespace action
{
using FsAction = ActionType<
	typing::ValWrapper<ActionRuleType, ActionRuleType::PATH_BENEATH>>;
using NetAction = ActionType<
	typing::ValWrapper<ActionRuleType, ActionRuleType::NET_PORT>>;

using AllAction = ActionType<typing::ValWrapper<
	ActionRuleType,
	ActionRuleType::PATH_BENEATH,
	ActionRuleType::NET_PORT>>;

/**
 * Dummy invalid action
 *
 * This is a dummy action which does not do anything with regards to landlock.
 * It is used as a fallback definition for actions which aren't supported in the
 * API/ABI that is present at compile time.
 *
 * It is harmless to use or combine this action with other actions as it does
 * not enable any action bits and does not raise the minimum required ABI
 * version.
 *
 * @internal Adding new types: check linux/landlock.h for new symbols and the
 * ABI version in which they were introduced. For the new ABI version, add a if
 * preprocessor block, creating the new symbols if LLPP_BUILD_LANDLOCK_ABI >=
 * new_abi_version and use = INVALID_ACTION for the else branch for each of the
 * types as a fallback for older systems.
 */
constexpr static AllAction INVALID_ACTION{0, std::numeric_limits<int>::min()};
constexpr static FsAction INVALID_ACTION_FS{
	INVALID_ACTION.type_code(), INVALID_ACTION.min_abi()
};
constexpr static NetAction INVALID_ACTION_NET{
	INVALID_ACTION.type_code(), INVALID_ACTION.min_abi()
};

#if LLPP_BUILD_LANDLOCK_API >= 1
constexpr static FsAction FS_EXECUTE{LANDLOCK_ACCESS_FS_EXECUTE, 1};
constexpr static FsAction FS_WRITE_FILE{LANDLOCK_ACCESS_FS_WRITE_FILE, 1};
constexpr static FsAction FS_READ_FILE{LANDLOCK_ACCESS_FS_READ_FILE, 1};
constexpr static FsAction FS_READ_DIR{LANDLOCK_ACCESS_FS_READ_DIR, 1};

constexpr static FsAction FS_REMOVE_DIR{LANDLOCK_ACCESS_FS_REMOVE_DIR, 1};
constexpr static FsAction FS_REMOVE_FILE{LANDLOCK_ACCESS_FS_REMOVE_FILE, 1};
constexpr static FsAction FS_MAKE_CHAR{LANDLOCK_ACCESS_FS_MAKE_CHAR, 1};
constexpr static FsAction FS_MAKE_DIR{LANDLOCK_ACCESS_FS_MAKE_DIR, 1};
constexpr static FsAction FS_MAKE_SOCK{LANDLOCK_ACCESS_FS_MAKE_SOCK, 1};
constexpr static FsAction FS_MAKE_FIFO{LANDLOCK_ACCESS_FS_MAKE_FIFO, 1};
constexpr static FsAction FS_MAKE_BLOCK{LANDLOCK_ACCESS_FS_MAKE_BLOCK, 1};
constexpr static FsAction FS_MAKE_SYM{LANDLOCK_ACCESS_FS_MAKE_SYM, 1};
#else
constexpr static FsAction FS_EXECUTE = INVALID_ACTION_FS;
constexpr static FsAction FS_WRITE_FILE = INVALID_ACTION_FS;
constexpr static FsAction FS_READ_FILE = INVALID_ACTION_FS;
constexpr static FsAction FS_READ_DIR = INVALID_ACTION_FS;

constexpr static FsAction FS_REMOVE_DIR = INVALID_ACTION_FS;
constexpr static FsAction FS_REMOVE_FILE = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_CHAR = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_DIR = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_SOCK = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_FIFO = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_BLOCK = INVALID_ACTION_FS;
constexpr static FsAction FS_MAKE_SYM = INVALID_ACTION_FS;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 2
constexpr static FsAction FS_REFER{LANDLOCK_ACCESS_FS_REFER, 2};
#else
constexpr static FsAction FS_REFER = INVALID_ACTION_FS;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 3
constexpr static FsAction FS_TRUNCATE{LANDLOCK_ACCESS_FS_TRUNCATE, 3};
#else
constexpr static FsAction FS_TRUNCATE = INVALID_ACTION_FS;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 4
constexpr static NetAction NET_BIND_TCP{LANDLOCK_ACCESS_NET_BIND_TCP, 4};
constexpr static NetAction NET_CONNECT_TCP{LANDLOCK_ACCESS_NET_CONNECT_TCP, 4};
#else
constexpr static NetAction NET_BIND_TCP = INVALID_ACTION_NET;
constexpr static NetAction NET_CONNECT_TCP = INVALID_ACTION_NET;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 5
constexpr static FsAction FS_IOCTL_DEV{LANDLOCK_ACCESS_FS_IOCTL_DEV, 5};
#else
constexpr static FsAction FS_IOCTL_DEV = INVALID_ACTION_FS;
#endif
} // namespace action

/**
 * Filter actions by ABI and fold them with operator|
 *
 * This function combines an arbitrary number of actions, but filters
 * out any actions whose min_abi() is bigger than the supplied max_abi,
 * allowing to filter out unsupported actions for any given ABI version.
 *
 * The resulting action type supports the union of rule types of all supplied
 * actions.
 */
template <typename RuleT, RuleT... supp_lhs, typename... ActionT>
constexpr ActionType<typing::MultiUnionT<
	RuleT,
	ActionType<typing::ValWrapper<RuleT, supp_lhs...>>,
	ActionT...>>
join(int max_abi,
     const ActionType<typing::ValWrapper<RuleT, supp_lhs...>>& action,
     const ActionT&... actions)
{
	if (action.min_abi() > max_abi) {
		return join(max_abi, actions...);
	}

	return action | join(max_abi, actions...);
}

template <typename RuleT, RuleT... supp_lhs, RuleT... supp_rhs>
constexpr ActionType<typing::UnionT<
	RuleT,
	typing::ValWrapper<RuleT, supp_lhs...>,
	typing::ValWrapper<RuleT, supp_rhs...>>>
join(int max_abi,
     const ActionType<typing::ValWrapper<RuleT, supp_lhs...>>& lhs,
     const ActionType<typing::ValWrapper<RuleT, supp_rhs...>>& rhs)
{
	if (lhs.min_abi() > max_abi && rhs.min_abi() > max_abi) {
		return {action::INVALID_ACTION.type_code(),
			action::INVALID_ACTION.min_abi()};
	}

	if (lhs.min_abi() > max_abi) {
		return rhs;
	}

	if (rhs.min_abi() > max_abi) {
		return lhs;
	}

	return lhs | rhs;
}

template <typename RuleT, RuleT supp>
ActionType<typing::ValWrapper<RuleT, supp>>
join(int max_abi,
     const std::vector<ActionType<typing::ValWrapper<RuleT, supp>>>& actions)
{
	using AT = ActionType<typing::ValWrapper<RuleT, supp>>;
	AT atype = reduce<RuleT, supp>(action::INVALID_ACTION);

	for (const AT& act : actions) {
		if (act.min_abi() <= max_abi) {
			atype |= act;
		}
	}

	return atype;
}
} // namespace landlock
