#include <cstdint>
#include <limits>

#include "ll/ActionType.hpp"

#include "test.hpp"

using landlock::ActionType;

TEST_CASE("ActionType::construction")
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

	ActionType atype{type_code, min_abi};

	CHECK(atype.type_code() == type_code);
	CHECK(atype.min_abi() == min_abi);

	SECTION("copy construction")
	{
		const ActionType copy{atype};
		CHECK(copy.type_code() == atype.type_code());
		CHECK(copy.min_abi() == atype.min_abi());
	}

	SECTION("operator= copy")
	{
		ActionType copy = landlock::action::INVALID_ACTION;
		copy = atype;
		CHECK(copy.type_code() == atype.type_code());
		CHECK(copy.min_abi() == atype.min_abi());
	}

	// NOLINTBEGIN(*-move-const-arg)
	SECTION("move construction")
	{
		const ActionType moved{std::move(atype)};
		CHECK(moved.type_code() == type_code);
		CHECK(moved.min_abi() == min_abi);
	}

	SECTION("operator= move")
	{
		ActionType moved = landlock::action::INVALID_ACTION;
		moved = std::move(atype);
		CHECK(moved.type_code() == type_code);
		CHECK(moved.min_abi() == min_abi);
	}
	// NOLINTEND(*-move-const-arg)
}

TEST_CASE("ActionType::combination")
{
	ActionType atype{0x01, 1};

	const std::uint64_t other_bit = GENERATE(take(3, random(2, 64)));
	const std::uint64_t other_type = 1U << other_bit;
	const int other_abi =
		GENERATE(take(3, random(2, std::numeric_limits<int>::max())));
	const ActionType other_action{other_type, other_abi};

	SECTION("operator|=")
	{
		atype |= other_action;
		CHECK(atype.type_code() == (0x01U | other_type));
		CHECK(atype.min_abi() == other_abi);
	}

	SECTION("operator|")
	{
		const ActionType new_action = atype | other_action;
		CHECK(new_action.type_code() == (0x01U | other_type));
		CHECK(new_action.min_abi() == other_abi);
	}
}

TEST_CASE("ActionType::join")
{
	constexpr std::size_t MAX_TYPES{10};
	std::vector<ActionType> types;
	for (std::size_t i = 0; i < MAX_TYPES; ++i) {
		types.emplace_back(1U << i, static_cast<int>(i));
	}

	const int max_abi = GENERATE(take(3, random(0UL, MAX_TYPES - 1)));

	const ActionType type = ActionType::join(
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

TEST_CASE("ActionType::comparison")
{
	const std::uint64_t bit1 = GENERATE(take(3, random(20, 62)));
	const std::uint64_t type1 = std::uint64_t{1U} << bit1;
	const int abi1 = GENERATE(
		take(3, random(10, std::numeric_limits<int>::max() - 1))
	);

	const ActionType at1{type1, abi1};

	SECTION("equals")
	{
		const ActionType at2 = at1;
		CHECK(at1 == at2);
		CHECK_FALSE(at1 != at2);
	}

	SECTION("not equals")
	{
		ActionType at2{0, -1};

		SECTION("different type")
		{
			const std::uint64_t bit2 =
				GENERATE(take(3, random(0, 9)));
			const std::uint64_t type2 = std::uint64_t{1U} << bit2;

			at2 = ActionType{type2, at1.min_abi()};
		}

		SECTION("different min_abi")
		{
			const int abi2 = GENERATE(take(3, random(0, 9)));

			at2 = ActionType{at1.type_code(), abi2};
		}

		CHECK(at1 != at2);
		CHECK_FALSE(at1 == at2);
	}
}
