#pragma once

#include <ll/CodedType.hpp>

namespace landlock
{
/**
 * Scope for limiting access to outside resources
 */
using Scope = CodedType<typing::ValWrapper<std::uint64_t, 0>, std::uint64_t>;

/**
 * Definitions for Landlock scopes
 */
namespace scope
{
constexpr static Scope INVALID_SCOPE{0, 0};

// NOLINTBEGIN(*-macro-usage)
#define DECL_SCOPE(scope, abi)                                                 \
	constexpr static Scope scope                                           \
	{                                                                      \
		(LANDLOCK_SCOPE_##scope), abi                                  \
	}
#define DECL_INVALID_SCOPE(scope) constexpr static Scope scope = INVALID_SCOPE

#if LLPP_BUILD_LANDLOCK_API >= 6
# define DECL_SCOPE_ABI6(scope) DECL_SCOPE(scope, 6)
#else
# define DECL_SCOPE_ABI6(scope) DECL_INVALID_SCOPE(scope)
#endif

DECL_SCOPE_ABI6(ABSTRACT_UNIX_SOCKET);
DECL_SCOPE_ABI6(SIGNAL);

#undef DECL_SCOPE
// NOLINTEND(*-macro-usage)
} // namespace scope
} // namespace landlock
