#include "ll/typing.hpp"

#include <iostream>
#include <type_traits>

#include "test.hpp"

using namespace landlock;
using typing::ValWrapper;

// NOLINTBEGIN(*-magic-numbers)

TEST_CASE("typing::is_element")
{
	CHECK(typing::is_element<int, 5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10>());
	CHECK_FALSE(typing::is_element<int, 5, 1, 2, 3, 4, 6, 7, 8, 9, 10>());
}

TEST_CASE("typing::Combine")
{
	CHECK(std::is_same_v<
		ValWrapper<int, 1, 2, 3>,
		typing::CombineT<int, 1, ValWrapper<int, 2, 3>>>);
}

TEST_CASE("typing::Union")
{
	using Union1 = typing::UnionT<
		int,
		ValWrapper<int, 1, 2, 3, 4, 5>,
		ValWrapper<int, 0, 1, 2, 3>>;

	using Union2 = typing::
		UnionT<int, ValWrapper<int, 5, 4, 3, 2, 1>, ValWrapper<int, 1>>;

	using Union3 = typing::
		UnionT<int, ValWrapper<int, 1>, ValWrapper<int, 5, 4, 3, 2, 1>>;

	std::cout << "Union1 = " << typeid(Union1).name() << '\n'
		  << "Union2 = " << typeid(Union2).name() << '\n'
		  << "Union3 = " << typeid(Union3).name() << '\n';

	CHECK(std::is_same_v<ValWrapper<int, 1, 2, 3>, Union1>);
	CHECK(std::is_same_v<ValWrapper<int, 1>, Union2>);
	CHECK(std::is_same_v<ValWrapper<int, 1>, Union3>);
}

// NOLINTEND(*-magic-numbers)
