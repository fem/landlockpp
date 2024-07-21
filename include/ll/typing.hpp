/**
 * @file typing.hpp Helper functions for types and template metaprogramming
 */
#pragma once

#if __cplusplus < 202002L
# define LLPP_CONSTEVAL constexpr
#else
# define LLPP_CONSTEVAL consteval
#endif

namespace landlock::typing
{
/**
 * Return differing types depending on bool condition
 *
 * If Cond evaluates to true, TrueT is returned as ::type. Otherwise, FalseT is
 * returned.
 */
template <bool Cond, typename TrueT, typename FalseT>
struct IfElse;

template <typename TrueT, typename FalseT>
struct IfElse<true, TrueT, FalseT> {
	using type = TrueT;
};

template <typename TrueT, typename FalseT>
struct IfElse<false, TrueT, FalseT> {
	using type = FalseT;
};

template <bool Cond, typename TrueT, typename FalseT>
using IfElseT = typename IfElse<Cond, TrueT, FalseT>::type;

/**
 * Wrapper for value pack as a single template parameter
 */
template <typename T, T... vals>
struct ValWrapper {
	using type = T;
};

/**
 * Combine a value and a value wrapper
 *
 * This returns a value wrapper with the first value and the values of the
 * value wrapper. Use CombineT as a convenience wrapper, which immediately
 * returns the type.
 *
 * The corresponding convenience wrapper is CombineT.
 */
template <typename T, T v, typename U>
struct Combine;

template <typename T, T v, typename U>
using CombineT = typename Combine<T, v, U>::type;

/**
 * Check whether item is contained in the set {set_1} + set_n
 */
template <typename T, T item, T set_1, T... set_n>
LLPP_CONSTEVAL bool is_element();
template <typename T, T item>
LLPP_CONSTEVAL bool is_element();

template <typename T, T item, T set_1, T... set_n>
LLPP_CONSTEVAL bool is_element()
{
	return item == set_1 or is_element<T, item, set_n...>();
}

template <typename T, T item>
LLPP_CONSTEVAL bool is_element()
{
	return false;
}

/**
 * Calculate the union between value packs U1 and U2
 *
 * U1 and U2 must be ValWrappers of type T. The resulting type() is a value
 * wrapper containing exactly the values that are both in U1 and U2.
 *
 * The corresponding convenience wrapper is UnionT.
 */
template <typename T, typename U1, typename U2>
struct Union;

template <typename T, typename U1, typename U2>
using UnionT = typename Union<T, U1, U2>::type;

template <typename T, T lv1, T... lvs, T... rvs>
struct Union<T, ValWrapper<T, lv1, lvs...>, ValWrapper<T, rvs...>> {
	constexpr static bool ADD_LV1 = is_element<T, lv1, rvs...>();
	using type_true = CombineT<
		T,
		lv1,
		UnionT<T, ValWrapper<T, lvs...>, ValWrapper<T, rvs...>>>;
	using type_false =
		UnionT<T, ValWrapper<T, lvs...>, ValWrapper<T, rvs...>>;

	using type = IfElseT<ADD_LV1, type_true, type_false>;
};

template <typename T, T lv1, T... rvs>
struct Union<T, ValWrapper<T, lv1>, ValWrapper<T, rvs...>> {
	constexpr static bool ADD_LV1 = is_element<T, lv1, rvs...>();

	using type_true = ValWrapper<T, lv1>;
	using type_false = ValWrapper<T>;

	using type = IfElseT<ADD_LV1, type_true, type_false>;
};

template <typename T, T v, T... us>
struct Combine<T, v, ValWrapper<T, us...>> {
	using type = ValWrapper<T, v, us...>;
};
} // namespace landlock::typing
