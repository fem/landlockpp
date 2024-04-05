#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ostream>
#include <vector>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/config.h>

namespace landlock
{
/**
 * Landlock action type
 *
 * This represents a type of action that can be secured by landlock. Each action
 * is associated with a type code and a minimum ABI version required to secure
 * this action using landlock.
 */
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

	/**
	 * Filter actions by ABI and fold them with operator|
	 *
	 * This function combines an arbitrary number of actions, but filters
	 * out any actions whose min_abi() is bigger than the supplied max_abi,
	 * allowing to filter out unsupported actions for any given ABI version.
	 *
	 * @internal This function and its base case are defined at the end of
	 * the file as they require action::INVALID_ACTION to be defined as
	 * default.
	 */
	template <typename... T>
	constexpr static ActionType
	join(int max_abi, const ActionType& action1, const T&... actions);

	static ActionType
	join(int max_abi, const std::vector<ActionType>& actions);

	/// Base case of join()
	constexpr static ActionType join(int max_abi, const ActionType& action);

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
} // namespace landlock

constexpr landlock::ActionType operator|(
	const landlock::ActionType& lhs, const landlock::ActionType& rhs
) noexcept
{
	return {lhs.type_code() | rhs.type_code(),
		std::max(lhs.min_abi(), rhs.min_abi())};
}

constexpr bool operator==(
	const landlock::ActionType& lhs, const landlock::ActionType& rhs
) noexcept
{
	return lhs.type_code() == rhs.type_code() &&
	       lhs.min_abi() == rhs.min_abi();
}

constexpr bool operator!=(
	const landlock::ActionType& lhs, const landlock::ActionType& rhs
) noexcept
{
	return not(lhs == rhs);
}

std::ostream&
operator<<(std::ostream& out, const landlock::ActionType& atype) noexcept;

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
constexpr static ActionType INVALID_ACTION{0, std::numeric_limits<int>::min()};

#if LLPP_BUILD_LANDLOCK_API >= 1
constexpr static ActionType FS_EXECUTE{LANDLOCK_ACCESS_FS_EXECUTE, 1};
constexpr static ActionType FS_WRITE_FILE{LANDLOCK_ACCESS_FS_WRITE_FILE, 1};
constexpr static ActionType FS_READ_FILE{LANDLOCK_ACCESS_FS_READ_FILE, 1};
constexpr static ActionType FS_READ_DIR{LANDLOCK_ACCESS_FS_READ_DIR, 1};

constexpr static ActionType FS_REMOVE_DIR{LANDLOCK_ACCESS_FS_REMOVE_DIR, 1};
constexpr static ActionType FS_REMOVE_FILE{LANDLOCK_ACCESS_FS_REMOVE_FILE, 1};
constexpr static ActionType FS_MAKE_CHAR{LANDLOCK_ACCESS_FS_MAKE_CHAR, 1};
constexpr static ActionType FS_MAKE_DIR{LANDLOCK_ACCESS_FS_MAKE_DIR, 1};
constexpr static ActionType FS_MAKE_SOCK{LANDLOCK_ACCESS_FS_MAKE_SOCK, 1};
constexpr static ActionType FS_MAKE_FIFO{LANDLOCK_ACCESS_FS_MAKE_FIFO, 1};
constexpr static ActionType FS_MAKE_BLOCK{LANDLOCK_ACCESS_FS_MAKE_BLOCK, 1};
constexpr static ActionType FS_MAKE_SYM{LANDLOCK_ACCESS_FS_MAKE_SYM, 1};
#else
constexpr static ActionType FS_EXECUTE = INVALID_ACTION;
constexpr static ActionType FS_WRITE_FILE = INVALID_ACTION;
constexpr static ActionType FS_READ_FILE = INVALID_ACTION;
constexpr static ActionType FS_READ_DIR = INVALID_ACTION;

constexpr static ActionType FS_REMOVE_DIR = INVALID_ACTION;
constexpr static ActionType FS_REMOVE_FILE = INVALID_ACTION;
constexpr static ActionType FS_MAKE_CHAR = INVALID_ACTION;
constexpr static ActionType FS_MAKE_DIR = INVALID_ACTION;
constexpr static ActionType FS_MAKE_SOCK = INVALID_ACTION;
constexpr static ActionType FS_MAKE_FIFO = INVALID_ACTION;
constexpr static ActionType FS_MAKE_BLOCK = INVALID_ACTION;
constexpr static ActionType FS_MAKE_SYM = INVALID_ACTION;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 2
constexpr static ActionType FS_REFER{LANDLOCK_ACCESS_FS_REFER, 2};
#else
constexpr static ActionType FS_REFER = INVALID_ACTION;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 3
constexpr static ActionType FS_TRUNCATE{LANDLOCK_ACCESS_FS_TRUNCATE, 3};
#else
constexpr static ActionType FS_TRUNCATE = INVALID_ACTION;
#endif

#if LLPP_BUILD_LANDLOCK_API >= 4
constexpr static ActionType NET_BIND_TCP{LANDLOCK_ACCESS_NET_BIND_TCP, 4};
constexpr static ActionType NET_CONNECT_TCP{LANDLOCK_ACCESS_NET_CONNECT_TCP, 4};
#else
constexpr static ActionType NET_BIND_TCP = INVALID_ACTION;
constexpr static ActionType NET_CONNECT_TCP = INVALID_ACTION;
#endif
} // namespace action

template <typename... T>
constexpr ActionType
ActionType::join(int max_abi, const ActionType& action, const T&... actions)
{
	if (action.min_abi_ > max_abi) {
		return join(max_abi, actions...);
	}

	return action | join(max_abi, actions...);
}

constexpr ActionType ActionType::join(int max_abi, const ActionType& action)
{
	if (action.min_abi_ > max_abi) {
		return action::INVALID_ACTION;
	}

	return action;
}
} // namespace landlock
