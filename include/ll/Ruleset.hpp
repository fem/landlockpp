#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

extern "C" {
#include <linux/landlock.h>
}

#include <ll/ActionType.hpp>
#include <ll/Rule.hpp>
#include <ll/RuleType.hpp>
#include <ll/config.h>
#include <ll/typing.hpp>

namespace landlock
{
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
	template <ActionRuleType supp>
	using ActionVec = std::vector<ActionType<supp>>;
	using RuleVariant = std::variant<PathBeneathRule, NetPortRule>;

	/**
	 * Create a new ruleset for filtering the specified filesystem actions
	 *
	 * This constructs a new ruleset and creates a ruleset file descriptor
	 * using the Linux landlock API.
	 *
	 * @param handled_access_fs Actions to filter with this ruleset. Any
	 * actions not listed here are not filtered.
	 *
	 * @throws std::system_error If the syscall fails
	 */
	explicit Ruleset(
		const ActionVec<ActionRuleType::PATH_BENEATH>&
			handled_access_fs = {},
		const ActionVec<ActionRuleType::NET_PORT>& handled_access_net =
			{}
	);
	Ruleset(const Ruleset&) = delete;
	Ruleset& operator=(const Ruleset&) = delete;
	Ruleset(Ruleset&&) = delete;
	Ruleset& operator=(Ruleset&&) = delete;
	~Ruleset();

	/**
	 * Return whether Landlock support is enabled on the system
	 *
	 * Since the library is designed to provide best-effort safety, failure
	 * to set up Landlock due to lack of support doesn't cause an exception.
	 * Instead, this function returns whether support is available so
	 * informational output can be printed as needed.
	 */
	[[nodiscard]] constexpr bool landlock_enabled() const noexcept
	{
		return abi_version_ > 0;
	}

	/**
	 * Get the probed Landlock ABI version
	 */
	[[nodiscard]] int abi_version() const noexcept
	{
		return abi_version_;
	}

	/**
	 * Get the effective Landlock ABI version
	 *
	 * The effective ABI version is the minimum of the ABI version of the
	 * running kernel and the API version of the headers the library was
	 * compiled with. Thus, it represents the maximum ABI version for which
	 * working Landlock rules are available and can be registered in the
	 * kernel's Landlock module.
	 */
	[[nodiscard]] constexpr int effective_abi_version() const noexcept
	{
		return std::min(abi_version_, LLPP_BUILD_LANDLOCK_API);
	}

	/**
	 * Add a new rule to the ruleset
	 *
	 * The rule's generate() method is called to obtain all rules to add to
	 * the ruleset.
	 */
	template <
		typename Self,
		typename AttrT,
		ActionRuleType supp,
		int min_abi>
	Ruleset& add_rule(Rule<Self, AttrT, supp, min_abi>&& rule)
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
	/**
	 * Read and store the running ABI version from the Landlock API
	 *
	 * @return true, if Landlock is available; false, otherwise
	 */
	bool read_abi_version();

	/**
	 * Initialize the Landlock Ruleset
	 */
	void init_ruleset(
		const ActionVec<ActionRuleType::PATH_BENEATH>&
			handled_access_fs,
		const ActionVec<ActionRuleType::NET_PORT>& handled_access_net
	);

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
