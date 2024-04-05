#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include "ll/ActionType.hpp"
#include "ll/Rule.hpp"

namespace landlock
{
PathBeneathRule::~PathBeneathRule()
{
	for (int path_fd : path_fds_) {
		// Not much we can do if this fails, so ignore the result
		::close(path_fd);
	}
}

PathBeneathRule::AttrVec PathBeneathRule::generate(int max_abi) const noexcept
{
	if (max_abi < MIN_ABI) {
		return {};
	}

	const ActionType type = fold_actions(max_abi);

	AttrVec res;
	res.reserve(path_fds_.size());
	for (const int path_fd : path_fds_) {
		Attr attr;
		attr.allowed_access = type.type_code();
		attr.parent_fd = path_fd;
		res.push_back(attr);
	}

	return res;
}

PathBeneathRule& PathBeneathRule::add_path(const std::filesystem::path& path)
{
	const int path_fd =
		::open(path.c_str(), O_PATH | O_CLOEXEC); // NOLINT(*-vararg)
	if (path_fd < 0) {
		throw std::system_error{
			std::error_code{errno, std::system_category()}
		};
	}
	path_fds_.push_back(path_fd);
	return *this;
}

NetPortRule& NetPortRule::add_port(std::uint16_t port)
{
	ports_.push_back(port);
	return *this;
}

NetPortRule::AttrVec NetPortRule::generate([[maybe_unused]] int max_abi
) const noexcept
{
#if LLPP_BUILD_LANDLOCK_API >= 4
	const ActionType type = fold_actions(max_abi);

	if (type.type_code() == 0) {
		return {};
	}

	AttrVec res;
	res.reserve(ports_.size());
	for (const std::uint16_t port : ports_) {
		Attr attr;
		attr.allowed_access = type.type_code();
		attr.port = port;
		res.push_back(attr);
	}

	return res;
#else
	return {};
#endif
}
} // namespace landlock
