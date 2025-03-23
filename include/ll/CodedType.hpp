#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <vector>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/config.h>
#include <ll/typing.hpp>

namespace landlock
{
/**
 * Representator for bitfield-encoded types of some sort with ABI restriction
 *
 * This is a common base class for many kinds of enum bitfields used in the
 * Landlock API. In these, each member corresponds to a single bit in an
 * underlying type. These can be combined to represent combined types with
 * multiple bits set, which is needed for rule creation. Since some bits are
 * not available in all Landlock ABIs, each coded type is combined with an ABI
 * restriction, indicating which minimum ABI is needed for this type to work.
 *
 * The class supports compile-time creation and combination of values, raising
 * the combination's minimum ABI to the maximum of all combined subtypes.
 */
template <typename SuppT, typename CodeT = std::uint64_t>
class CodedType
{
public:
	using Code = CodeT;

	// NOLINTNEXTLINE(*-easily-swappable-parameters)
	constexpr CodedType(Code type_code, int min_abi) :
		type_code_(type_code), min_abi_(min_abi)
	{
	}

	constexpr CodedType(const CodedType&) = default;
	constexpr CodedType& operator=(const CodedType&) = default;
	constexpr CodedType(CodedType&&) = default;
	constexpr CodedType& operator=(CodedType&&) = default;
	~CodedType() = default;

	/**
	 * Combine two coded types
	 *
	 * The combination is the bitwise OR of the type codes (each type is
	 * represented by one bit in the Linux headers) and the minimum ABI
	 * version of the resulting coded type is raised to the maximum of the
	 * two actions.
	 */
	constexpr CodedType& operator|=(const CodedType& other) noexcept
	{
		type_code_ |= other.type_code_;
		min_abi_ = std::max(min_abi_, other.min_abi_);
		return *this;
	}

	[[nodiscard]] constexpr Code type_code() const noexcept
	{
		return type_code_;
	}
	[[nodiscard]] constexpr int min_abi() const noexcept
	{
		return min_abi_;
	}

private:
	Code type_code_;
	int min_abi_;
};

namespace typing
{
template <typename T, T... supp>
struct Unwrap<CodedType<ValWrapper<T, supp...>>> {
	using type = ValWrapper<T, supp...>;
};
} // namespace typing

template <typename RuleT, RuleT reduce_to, RuleT... supp>
constexpr const CodedType<typing::ValWrapper<RuleT, reduce_to>>&
reduce(const CodedType<typing::ValWrapper<RuleT, supp...>>& action)
{
	static_assert(
		typing::is_element<RuleT, reduce_to, supp...>(),
		"Trying to reduce to unsupported type"
	);
	// TODO can we get rid of reinterpret_cast?
	return *reinterpret_cast<
		const CodedType<typing::ValWrapper<RuleT, reduce_to>>*>(&action
	);
}

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
constexpr CodedType<typing::MultiUnionT<
	RuleT,
	CodedType<typing::ValWrapper<RuleT, supp_lhs...>>,
	ActionT...>>
join(int max_abi,
     const CodedType<typing::ValWrapper<RuleT, supp_lhs...>>& action,
     const ActionT&... actions)
{
	if (action.min_abi() > max_abi) {
		return join(max_abi, actions...);
	}

	return action | join(max_abi, actions...);
}

template <typename RuleT, RuleT... supp_lhs, RuleT... supp_rhs>
constexpr CodedType<typing::UnionT<
	RuleT,
	typing::ValWrapper<RuleT, supp_lhs...>,
	typing::ValWrapper<RuleT, supp_rhs...>>>
join(int max_abi,
     const CodedType<typing::ValWrapper<RuleT, supp_lhs...>>& lhs,
     const CodedType<typing::ValWrapper<RuleT, supp_rhs...>>& rhs)
{
	if (lhs.min_abi() > max_abi && rhs.min_abi() > max_abi) {
		return {0, 0};
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
CodedType<typing::ValWrapper<RuleT, supp>>
join(int max_abi,
     const std::vector<CodedType<typing::ValWrapper<RuleT, supp>>>& actions)
{
	using AT = CodedType<typing::ValWrapper<RuleT, supp>>;
	AT atype{0, 0};

	for (const AT& act : actions) {
		if (act.min_abi() <= max_abi) {
			atype |= act;
		}
	}

	return atype;
}
} // namespace landlock

template <typename RuleT, RuleT... supp_lhs, RuleT... supp_rhs>
constexpr landlock::CodedType<landlock::typing::UnionT<
	RuleT,
	landlock::typing::ValWrapper<RuleT, supp_lhs...>,
	landlock::typing::ValWrapper<RuleT, supp_rhs...>>>
operator|(
	const landlock::CodedType<
		landlock::typing::ValWrapper<RuleT, supp_lhs...>>& lhs,
	const landlock::CodedType<
		landlock::typing::ValWrapper<RuleT, supp_rhs...>>& rhs
) noexcept
{
	return {lhs.type_code() | rhs.type_code(),
		std::max(lhs.min_abi(), rhs.min_abi())};
}

template <typename T, typename U>
constexpr bool operator==(
	[[maybe_unused]] const landlock::CodedType<T>& lhs,
	[[maybe_unused]] const landlock::CodedType<U>& rhs
) noexcept
{
	// Actual equality is checked in template specializations
	return false;
}

template <typename supported>
constexpr bool operator==(
	const landlock::CodedType<supported>& lhs,
	const landlock::CodedType<supported>& rhs
) noexcept
{
	return lhs.type_code() == rhs.type_code() &&
	       lhs.min_abi() == rhs.min_abi();
}

template <typename T, typename U>
constexpr bool operator!=(
	const landlock::CodedType<T>& lhs, const landlock::CodedType<U>& rhs
) noexcept
{
	return not(lhs == rhs);
}

template <typename supported>
std::ostream&
operator<<(std::ostream& out, const landlock::CodedType<supported>& atype)
{
	const auto fmt = out.flags();
	out << std::hex << std::setfill('0')
	    << std::setw(sizeof(std::uint64_t) * 2) << atype.type_code() << '/'
	    << std::dec << atype.min_abi();
	out.flags(fmt);
	return out;
}
