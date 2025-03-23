#pragma once

#include <limits>
#include <type_traits>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/CodedType.hpp>
#include <ll/config.h>
#include <ll/typing.hpp>

namespace landlock
{
/**
 * Compatible rules for action types
 */
// NOLINTNEXTLINE(*-enum-size)
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
template <ActionRuleType... SuppV>
using ActionType =
	CodedType<typing::ValWrapper<ActionRuleType, SuppV...>, std::uint64_t>;

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
using FsAction = ActionType<ActionRuleType::PATH_BENEATH>;
using NetAction = ActionType<ActionRuleType::NET_PORT>;

using AllAction =
	ActionType<ActionRuleType::PATH_BENEATH, ActionRuleType::NET_PORT>;

// NOLINTBEGIN(*-macro-usage)
#define DECL_ACTION(type, cls, name, abi)                                      \
	constexpr static type cls##_##name                                     \
	{                                                                      \
		(LANDLOCK_ACCESS_##cls##_##name), abi                          \
	}

#define DECL_INVALID_ACTION(type, cls, name)                                   \
	constexpr static type cls##_##name = INVALID_ACTION_##cls

#if LLPP_BUILD_LANDLOCK_API >= 1
# define DECL_ACTION_ABI1(type, cls, name) DECL_ACTION(type, cls, name, 1)
#else
# define DECL_ACTION_ABI1(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif

#if LLPP_BUILD_LANDLOCK_API >= 2
# define DECL_ACTION_ABI2(type, cls, name) DECL_ACTION(type, cls, name, 2)
#else
# define DECL_ACTION_ABI2(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif

#if LLPP_BUILD_LANDLOCK_API >= 3
# define DECL_ACTION_ABI3(type, cls, name) DECL_ACTION(type, cls, name, 3)
#else
# define DECL_ACTION_ABI3(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif

#if LLPP_BUILD_LANDLOCK_API >= 4
# define DECL_ACTION_ABI4(type, cls, name) DECL_ACTION(type, cls, name, 4)
#else
# define DECL_ACTION_ABI4(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif

#if LLPP_BUILD_LANDLOCK_API >= 5
# define DECL_ACTION_ABI5(type, cls, name) DECL_ACTION(type, cls, name, 5)
#else
# define DECL_ACTION_ABI5(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif

#if LLPP_BUILD_LANDLOCK_API >= 6
# define DECL_ACTION_ABI6(type, cls, name) DECL_ACTION(type, cls, name, 6)
#else
# define DECL_ACTION_ABI6(type, cls, name) DECL_INVALID_ACTION(type, cls, name)
#endif
// NOLINTEND(*-macro-usage)

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

DECL_ACTION_ABI1(FsAction, FS, EXECUTE);
DECL_ACTION_ABI1(FsAction, FS, WRITE_FILE);
DECL_ACTION_ABI1(FsAction, FS, READ_FILE);
DECL_ACTION_ABI1(FsAction, FS, READ_DIR);
DECL_ACTION_ABI1(FsAction, FS, REMOVE_DIR);
DECL_ACTION_ABI1(FsAction, FS, REMOVE_FILE);
DECL_ACTION_ABI1(FsAction, FS, MAKE_CHAR);
DECL_ACTION_ABI1(FsAction, FS, MAKE_DIR);
DECL_ACTION_ABI1(FsAction, FS, MAKE_REG);
DECL_ACTION_ABI1(FsAction, FS, MAKE_SOCK);
DECL_ACTION_ABI1(FsAction, FS, MAKE_FIFO);
DECL_ACTION_ABI1(FsAction, FS, MAKE_BLOCK);
DECL_ACTION_ABI1(FsAction, FS, MAKE_SYM);

DECL_ACTION_ABI2(FsAction, FS, REFER);

DECL_ACTION_ABI3(FsAction, FS, TRUNCATE);

DECL_ACTION_ABI4(NetAction, NET, BIND_TCP);
DECL_ACTION_ABI4(NetAction, NET, CONNECT_TCP);

DECL_ACTION_ABI5(FsAction, FS, IOCTL_DEV);

#undef DECL_ACTION
#undef DECL_ACTION_ABI1
#undef DECL_ACTION_ABI2
#undef DECL_ACTION_ABI3
#undef DECL_ACTION_ABI4
#undef DECL_ACTION_ABI5
#undef DECL_ACTION_ABI6
} // namespace action
} // namespace landlock
