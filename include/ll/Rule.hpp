#pragma once

#include <filesystem>
#include <linux/landlock.h>
#include <type_traits>
#include <vector>

#include <ll/ActionType.hpp>
#include <ll/config.h>
#include <ll/typing.hpp>

namespace landlock
{
/**
 * Base landlock rule
 *
 * This class defines the common interface for landlock rules. All rule types
 * should derive from this interface. Each rule takes care of filtering out
 * unsupported actions from the API and returning only rules supported by the
 * supplied ABI version.
 *
 * @param Self Type of rule implementing this interface
 *
 * @param AttrT Return type of the generated attribute structs
 *
 * @param supported Action rule type of supported actions
 *
 * @param min_abi minimum ABI version required for this rule to work
 */
template <typename Self, typename AttrT, ActionRuleType supported, int min_abi>
class Rule
{
public:
	/// Resulting attribute type for the landlock API
	using Attr = AttrT;
	using AttrVec = std::vector<Attr>;
	using ReducedActionType = ActionType<supported>;
	using Base = Rule<Self, AttrT, supported, min_abi>;

	constexpr static int MIN_ABI = min_abi;
	constexpr static ActionRuleType SUPPORTED_ACTION_TYPE = supported;

	Rule() = default;
	Rule(const Rule&) = delete;
	Rule& operator=(const Rule&) = delete;
	Rule(Rule&&) = default;
	Rule& operator=(Rule&&) = default;
	virtual ~Rule() = default;

	/**
	 * Generate a list of attributes to add as rules to the ruleset
	 *
	 * If this rule type is not supported by the abi, this returns an empty
	 * std::vector. Otherwise, each entry is a rule to be added to the
	 * landlock ruleset.
	 */
	[[nodiscard]] virtual AttrVec generate(int max_abi) const noexcept = 0;

	/**
	 * Add an action to this rule
	 */
	template <typename RuleT, RuleT... action_supp>
	Self& add_action(ActionType<action_supp...> type)
	{
		static_assert(
			typing::is_element<
				RuleT,
				SUPPORTED_ACTION_TYPE,
				action_supp...>(),
			"Trying to add unsupported action for rule"
		);

		actions_.push_back(reduce<RuleT, SUPPORTED_ACTION_TYPE>(type));
		return *static_cast<Self*>(this);
	}

protected:
	/**
	 * Fold all supported actions into a single one
	 *
	 * Given the maximum supported ABI version, this function takes all
	 * action types with minimum ABI version below the maximum supported
	 * version and combines them into a single action type.
	 */
	[[nodiscard]] ReducedActionType fold_actions(int max_abi) const noexcept
	{
		return join(max_abi, actions_);
	}

private:
	std::vector<ReducedActionType> actions_;
};

/**
 * Rule for access to files beneath a path in the filesystem
 *
 * This rule controls access to files and directories beneath a path.
 */
class PathBeneathRule :
	public Rule<
		PathBeneathRule,
		landlock_path_beneath_attr,
		ActionRuleType::PATH_BENEATH,
		1>
{
public:
	PathBeneathRule() = default;
	PathBeneathRule(const PathBeneathRule&) = delete;
	PathBeneathRule& operator=(const PathBeneathRule&) = delete;
	PathBeneathRule(PathBeneathRule&&) = default;
	PathBeneathRule& operator=(PathBeneathRule&&) = default;
	~PathBeneathRule() override;

	[[nodiscard]] std::vector<Attr> generate(int max_abi
	) const noexcept override;

	PathBeneathRule& add_path(const std::filesystem::path& path);

private:
	std::vector<int> path_fds_;
};

/**
 * Rule for binding network ports
 *
 * This rule controls to which ports the process may bind to.
 */
class NetPortRule :
	public Rule<
		NetPortRule,
#if LLPP_BUILD_LANDLOCK_API >= 4
		landlock_net_port_attr
#else
		std::nullptr_t
#endif
		,
		ActionRuleType::NET_PORT,
		4>
{
public:
	NetPortRule() = default;
	NetPortRule(const NetPortRule&) = delete;
	NetPortRule& operator=(const NetPortRule&) = delete;
	NetPortRule(NetPortRule&&) = default;
	NetPortRule& operator=(NetPortRule&&) = default;
	~NetPortRule() override = default;

	[[nodiscard]] std::vector<Attr> generate(int max_abi
	) const noexcept override;

	NetPortRule& add_port(std::uint16_t port);

private:
	std::vector<std::uint16_t> ports_;
};
} // namespace landlock
