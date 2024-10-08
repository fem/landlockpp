/**
 * @file typing.hpp Helper functions for types and template metaprogramming
 */
#pragma once

#include <type_traits>

#if __cplusplus < 202002L
# define LLPP_CONSTEVAL constexpr
#else
# define LLPP_CONSTEVAL consteval
#endif

namespace landlock::typing
{
/**
 * Wrapper for value pack as a single template parameter
 */
template <typename T, T... vals>
struct ValWrapper {
	using type = T;
};

/**
 * Unwrap the first template parameter from a template type
 *
 * This must be specialized for all supported types. Given a type T<U> as a
 * parameter, the specialized struct must return U as its ::type.
 */
template <typename T>
struct Unwrap;

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

	using type = std::conditional_t<ADD_LV1, type_true, type_false>;
};

template <typename T, T lv1, T... rvs>
struct Union<T, ValWrapper<T, lv1>, ValWrapper<T, rvs...>> {
	constexpr static bool ADD_LV1 = is_element<T, lv1, rvs...>();

	using type_true = ValWrapper<T, lv1>;
	using type_false = ValWrapper<T>;

	using type = std::conditional_t<ADD_LV1, type_true, type_false>;
};

template <typename T, T v, T... us>
struct Combine<T, v, ValWrapper<T, us...>> {
	using type = ValWrapper<T, v, us...>;
};

/**
 * Perform Union on a pack of value packs
 */
template <typename T, typename... Us>
struct MultiUnion;

template <typename T, typename U, typename... Us>
struct MultiUnion<T, U, Us...> {
	using type =
		UnionT<T,
		       typename Unwrap<U>::type,
		       typename MultiUnion<T, Us...>::type>;
};

template <typename T, typename U>
struct MultiUnion<T, U> {
	using type = typename Unwrap<U>::type;
};

template <typename T, typename... Us>
using MultiUnionT = typename MultiUnion<T, Us...>::type;
} // namespace landlock::typing
