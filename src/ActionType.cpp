#include <iomanip>

#include "ll/ActionType.hpp"

namespace landlock
{
ActionType ActionType::join(int max_abi, const std::vector<ActionType>& actions)
{
	ActionType atype{action::INVALID_ACTION};
	for (const ActionType& act : actions) {
		if (act.min_abi_ <= max_abi) {
			atype |= act;
		}
	}

	return atype;
}
} // namespace landlock

std::ostream&
operator<<(std::ostream& out, const landlock::ActionType& atype) noexcept
{
	const auto fmt = out.flags();
	out << std::hex << std::setfill('0')
	    << std::setw(sizeof(std::uint64_t) * 2) << atype.type_code() << '/'
	    << std::dec << atype.min_abi();
	out.flags(fmt);
	return out;
}
