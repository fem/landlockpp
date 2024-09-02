#include "ll/Rule.hpp"
#include "ll/ActionType.hpp"
#include "ll/config.h"

#include "test.hpp"

using landlock::ActionType;
using landlock::NetPortRule;
using landlock::PathBeneathRule;
namespace action = landlock::action;

TEST_CASE("Rule::PathBeneathRule")
{
	PathBeneathRule pb_rule;
	NetPortRule np_rule;

	pb_rule.add_action(action::FS_EXECUTE)
		.add_action(action::FS_MAKE_DIR)
		.add_action(action::FS_MAKE_SOCK)
		.add_action(action::FS_REFER)
		.add_action(action::FS_TRUNCATE);
	pb_rule.add_path("/bin/sh");
	np_rule.add_action(action::NET_BIND_TCP);
	np_rule.add_port(42); // NOLINT(*-magic-numbers)

	SECTION("valid ABI")
	{
		action::FsAction expected_action_pb = action::INVALID_ACTION_FS;
		[[maybe_unused]] action::NetAction expected_action_np =
			action::INVALID_ACTION_NET;
		int abi = 0;

		SECTION("ABI 1")
		{
			expected_action_pb = action::FS_EXECUTE |
					     action::FS_MAKE_DIR |
					     action::FS_MAKE_SOCK;
			abi = 1;
		}

		SECTION("ABI 2")
		{
			expected_action_pb =
				action::FS_EXECUTE | action::FS_MAKE_DIR |
				action::FS_MAKE_SOCK | action::FS_REFER;
			abi = 2;
		}

		SECTION("ABI 3")
		{
			expected_action_pb =
				action::FS_EXECUTE | action::FS_MAKE_DIR |
				action::FS_MAKE_SOCK | action::FS_REFER |
				action::FS_TRUNCATE;
			abi = 3;
		}

		SECTION("ABI 4")
		{
			expected_action_pb =
				action::FS_EXECUTE | action::FS_MAKE_DIR |
				action::FS_MAKE_SOCK | action::FS_REFER |
				action::FS_TRUNCATE;
			expected_action_np = action::NET_BIND_TCP;
			abi = 4;
		}

		const PathBeneathRule::AttrVec pb_rules = pb_rule.generate(abi);
		const NetPortRule::AttrVec np_rules = np_rule.generate(abi);

		if (abi > 0) {
			REQUIRE(pb_rules.size() == 1);
			CHECK(pb_rules.at(0).allowed_access ==
			      expected_action_pb.type_code());
			CHECK(pb_rules.at(0).parent_fd > 0);
		}

#if LLPP_BUILD_LANDLOCK_API >= 4
		if (abi >= 4) {
			REQUIRE(np_rules.size() == 1);
			CHECK(np_rules.at(0).allowed_access ==
			      expected_action_np.type_code());
			// NOLINTNEXTLINE(*-magic-numbers)
			CHECK(np_rules.at(0).port == 42);
		}
#endif
	}

	SECTION("invalid ABI")
	{
		const PathBeneathRule::AttrVec pb_rules = pb_rule.generate(0);
		const NetPortRule::AttrVec np_rules = np_rule.generate(0);
		CHECK(pb_rules.empty());
		CHECK(np_rules.empty());
	}
}

TEST_CASE("Rule::NetPortRule")
{
	NetPortRule rule;

	// NOLINTNEXTLINE(*-magic-numbers)
	rule.add_port(42).add_port(666).add_port(1337).add_action(
		action::NET_BIND_TCP
	);

	SECTION("valid ABI")
	{
		[[maybe_unused]] action::NetAction expected_action =
			action::NET_BIND_TCP;
		int abi = 4;
		NetPortRule::AttrVec rules;

		rules = rule.generate(abi);

		if (abi > 0) {
#if LLPP_BUILD_LANDLOCK_API >= 4
			REQUIRE(rules.size() == 3);
			CHECK(rules.at(0).allowed_access ==
			      expected_action.type_code());
			CHECK(rules.at(0).port > 0);
#else
			CHECK(rules.empty());
#endif
		}
	}

// Otherwise, rules with invalid actions are generated
#if LLPP_BUILD_LANDLOCK_API >= 4
	SECTION("invalid ABI")
	{
		const NetPortRule::AttrVec rules = rule.generate(0);
		CHECK(rules.empty());
	}
#endif
}
