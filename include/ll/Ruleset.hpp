#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/Rule.hpp>
#include <ll/RuleType.hpp>

namespace landlock
{
class ActionType;

/**
 * Landlock ruleset abstraction
 *
 * The landlock ruleset is the main object in the Linux landlock API.
 * This ruleset stores a base configuration defining which types of actions
 * are protected using this ruleset as well as rules that should apply.
 */
class Ruleset
{
public:
	using RuleVariant = std::variant<PathBeneathRule, NetPortRule>;

	/**
	 * Create a new ruleset for filtering the specified filesystem actions
	 *
	 * This constructs a new ruleset and creates a ruleset file decriptor
	 * using the Linux landlock API.
	 *
	 * @param handled_access_fs Actions to filter with this ruleset. Any
	 * actions not listed here are not filtered.
	 *
	 * @throws std::system_error If the syscall fails
	 */
	explicit Ruleset(
		const std::vector<ActionType>& handled_access_fs = {},
		const std::vector<ActionType>& handled_access_net = {}
	);
	Ruleset(const Ruleset&) = delete;
	Ruleset& operator=(const Ruleset&) = delete;
	Ruleset(Ruleset&&) = delete;
	Ruleset& operator=(Ruleset&&) = delete;
	~Ruleset();

	/**
	 * Get the probed Landlock ABI version
	 */
	[[nodiscard]] int abi_version() const noexcept
	{
		return abi_version_;
	}

	/**
	 * Add a new rule to the ruleset
	 *
	 * The rule's generate() method is called to obtain all rules to add to
	 * the ruleset.
	 */
	template <typename Self, typename AttrT, int min_abi>
	Ruleset& add_rule(Rule<Self, AttrT, min_abi>&& rule)
	{
		for (const auto rule : rule.generate(abi_version_)) {
			add_rule_int(rule);
		}
		added_rules_.emplace_back(static_cast<Self&&>(std::move(rule)));
		return *this;
	}

	/**
	 * Enforce this ruleset
	 *
	 * Before calling this function, none of the landlock security
	 * mechanisms are activated. By calling this function, the Landlock API
	 * starts restricting actions as defined by this ruleset.
	 *
	 * @param set_no_new_privs Run prctl(1) to set NO_NEW_PRIVS
	 */
	void enforce(bool set_no_new_privs = true) const;

private:
	template <typename AttrT>
	void add_rule_int(const AttrT& rule)
	{
		const landlock_rule_type type =
			RuleType<std::remove_reference_t<AttrT>>::TYPE_CODE;
		if (type == INVALID_RULE_TYPE) {
			return;
		}

		const int res = landlock_add_rule(ruleset_fd_, type, &rule);
		assert_res(res);
	}

	/**
	 * Wrapper for the SYS_landlock_create_ruleset syscall
	 */
	static int landlock_create_ruleset(
		const landlock_ruleset_attr* attr,
		std::size_t size,
		std::uint32_t flags
	);

	/**
	 * Wrapper for the SYS_landlock_add_rule syscall
	 */
	static int landlock_add_rule(
		int ruleset_fd,
		landlock_rule_type rule_type,
		const void* rule_attr,
		std::uint32_t flags = 0
	);

	/**
	 * Wrapper for the SYS_landlock_restrict_self syscall
	 */
	static int
	landlock_restrict_self(int ruleset_fd, std::uint32_t flags = 0);

	static void assert_res(int res);

	int ruleset_fd_{-1};
	int abi_version_{0};

	std::vector<RuleVariant> added_rules_;
};
} // namespace landlock