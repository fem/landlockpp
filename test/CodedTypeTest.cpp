#include <cstdint>
#include <limits>

#include "ll/CodedType.hpp"

#include "test.hpp"

// NOLINTNEXTLINE(*-enum-size)
enum class CType : std::uint64_t {
	CODE1,
	CODE2,
};

using landlock::CodedType;
using landlock::typing::ValWrapper;
using CT = landlock::CodedType<ValWrapper<CType, CType::CODE1>>;

namespace
{
constexpr CodedType<ValWrapper<CType, CType::CODE1, CType::CODE2>>
	INVALID_CTYPE{0, 0};
} // namespace

TEST_CASE("CodedType::construction")
{
	const std::uint64_t type_code = GENERATE(
		take(3,
		     random(std::numeric_limits<std::uint64_t>::min(),
			    std::numeric_limits<std::uint64_t>::max()))
	);
	const int min_abi = GENERATE(
		take(3,
		     random(std::numeric_limits<std::uint64_t>::min(),
			    std::numeric_limits<std::uint64_t>::max()))
	);

	CT ctype{type_code, min_abi};

	CHECK(ctype.type_code() == type_code);
	CHECK(ctype.min_abi() == min_abi);

	SECTION("copy construction")
	{
		const CT copy{ctype};
		CHECK(copy.type_code() == ctype.type_code());
		CHECK(copy.min_abi() == ctype.min_abi());
	}

	SECTION("operator= copy")
	{
		CT copy = landlock::reduce<CType, CType::CODE1>(INVALID_CTYPE);
		copy = ctype;
		CHECK(copy.type_code() == ctype.type_code());
		CHECK(copy.min_abi() == ctype.min_abi());
	}

	// NOLINTBEGIN(*-move-const-arg)
	SECTION("move construction")
	{
		const CT moved{std::move(ctype)};
		CHECK(moved.type_code() == type_code);
		CHECK(moved.min_abi() == min_abi);
	}

	SECTION("operator= move")
	{
		CT moved = landlock::reduce<CType, CType::CODE1>(INVALID_CTYPE);
		moved = std::move(ctype);
		CHECK(moved.type_code() == type_code);
		CHECK(moved.min_abi() == min_abi);
	}
	// NOLINTEND(*-move-const-arg)
}

TEST_CASE("CodedType::combination")
{
	CT ctype{0x01, 1};

	const std::uint64_t other_bit = GENERATE(take(3, random(2, 64)));
	const std::uint64_t other_type = 1U << other_bit;
	const int other_abi =
		GENERATE(take(3, random(2, std::numeric_limits<int>::max())));
	const CT other_action{other_type, other_abi};

	SECTION("operator|=")
	{
		ctype |= other_action;
		CHECK(ctype.type_code() == (0x01U | other_type));
		CHECK(ctype.min_abi() == other_abi);
	}

	SECTION("operator|")
	{
		const CT new_action = ctype | other_action;
		CHECK(new_action.type_code() == (0x01U | other_type));
		CHECK(new_action.min_abi() == other_abi);
	}
}

TEST_CASE("CodedType::join")
{
	constexpr std::size_t MAX_TYPES{10};
	std::vector<CT> types;
	types.reserve(MAX_TYPES);
	for (std::size_t i = 0; i < MAX_TYPES; ++i) {
		types.emplace_back(1U << i, static_cast<int>(i));
	}

	const int max_abi = GENERATE(take(3, random(0UL, MAX_TYPES - 1)));

	const CT type = landlock::join(
		max_abi,
		types.at(0),
		types.at(1),
		types.at(2),
		types.at(3),
		types.at(4),
		types.at(5),
		types.at(6),
		types.at(7),
		types.at(8),
		types.at(9)
	);

	CHECK(type.min_abi() == max_abi);
	for (int i = 0; i < max_abi; ++i) {
		CHECK((type.type_code() & (1U << static_cast<unsigned>(i))) !=
		      0U);
	}
}

TEST_CASE("CodedType::comparison")
{
	const std::uint64_t bit1 = GENERATE(take(3, random(20, 62)));
	const std::uint64_t type1 = std::uint64_t{1U} << bit1;
	const int abi1 = GENERATE(
		take(3, random(10, std::numeric_limits<int>::max() - 1))
	);

	const CT ct1{type1, abi1};

	SECTION("equals")
	{
		const CT ct2 = ct1;
		CHECK(ct1 == ct2);
		CHECK_FALSE(ct1 != ct2);
	}

	SECTION("not equals")
	{
		CT ct2{0, -1};

		SECTION("different type")
		{
			const std::uint64_t bit2 =
				GENERATE(take(3, random(0, 9)));
			const std::uint64_t type2 = std::uint64_t{1U} << bit2;

			ct2 = CT{type2, ct1.min_abi()};
		}

		SECTION("different min_abi")
		{
			const int abi2 = GENERATE(take(3, random(0, 9)));

			ct2 = CT{ct1.type_code(), abi2};
		}

		CHECK(ct1 != ct2);
		CHECK_FALSE(ct1 == ct2);
	}
}
