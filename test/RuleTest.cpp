#include "ll/Rule.hpp"
#include "ll/ActionType.hpp"
#include "ll/config.h"

#include <catch2/catch_test_macros.hpp>
#include <limits>

using landlock::ActionType;
using landlock::NetPortRule;
using landlock::PathBeneathRule;
namespace action = landlock::action;

TEST_CASE("Rule::PathBeneathRule")
{
	PathBeneathRule rule;

	rule.add_action(action::FS_EXECUTE)
		.add_action(action::FS_MAKE_DIR)
		.add_action(action::FS_MAKE_SOCK)
		.add_action(action::FS_REFER);
	rule.add_path("/bin/sh");

	SECTION("valid ABI")
	{
		ActionType expected_action = action::INVALID_ACTION;
		int abi = 0;
		PathBeneathRule::AttrVec rules;

		SECTION("ABI 1")
		{
			expected_action = action::FS_EXECUTE |
					  action::FS_MAKE_DIR |
					  action::FS_MAKE_SOCK;
			abi = 1;
		}

		SECTION("ABI 2")
		{
			expected_action =
				action::FS_EXECUTE | action::FS_MAKE_DIR |
				action::FS_MAKE_SOCK | action::FS_REFER;
			abi = 2;
		}

		rules = rule.generate(abi);

		if (abi > 0) {
			REQUIRE(rules.size() == 1);
			CHECK(rules.at(0).allowed_access ==
			      expected_action.type_code());
			CHECK(rules.at(0).parent_fd > 0);
		}
	}

	SECTION("invalid ABI")
	{
		const PathBeneathRule::AttrVec rules = rule.generate(0);
		CHECK(rules.empty());
	}
}

TEST_CASE("Rule::NetPortRule")
{
	NetPortRule rule;

	// NOLINTNEXTLINE(*-magic-numbers)
	rule.add_port(42).add_port(666).add_port(1337);

	SECTION("valid ABI")
	{
		ActionType expected_action = action::INVALID_ACTION;
		int abi = 0;
		NetPortRule::AttrVec rules;

		SECTION("ABI 1")
		{
			expected_action = action::FS_EXECUTE |
					  action::FS_MAKE_DIR |
					  action::FS_MAKE_SOCK;
			abi = 1;
		}

		SECTION("ABI 2")
		{
			expected_action =
				action::FS_EXECUTE | action::FS_MAKE_DIR |
				action::FS_MAKE_SOCK | action::FS_REFER;
			abi = 2;
		}

		rules = rule.generate(abi);

		if (abi > 0) {
#if LLPP_BUILD_LANDLOCK_ABI >= 4
			REQUIRE(rules.size() == 1);
			CHECK(rules.at(0).allowed_access ==
			      expected_action.type_code());
			CHECK(rules.at(0).parent_fd > 0);
#else
			CHECK(rules.empty());
#endif
		}
	}

	SECTION("invalid ABI")
	{
		const NetPortRule::AttrVec rules = rule.generate(0);
		CHECK(rules.empty());
	}
}