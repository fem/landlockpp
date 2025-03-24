landlock_ver_code = '''
#include <iostream>
extern "C" {
#include <linux/landlock.h>
}

// NOLINTBEGIN(*-macro-usage)
// Resolve the Landlock ABI version supported by the currently present headers
// by probing for a few symbols known to have appeared in the given version
#if defined(LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET)
#define LLPP_BUILD_LANDLOCK_ABI 6
#elif defined(LANDLOCK_ACCESS_FS_IOCTL_DEV)
#define LLPP_BUILD_LANDLOCK_ABI 5
#elif defined(LANDLOCK_ACCESS_NET_BIND_TCP)
#define LLPP_BUILD_LANDLOCK_ABI 4
#elif defined(LANDLOCK_ACCESS_FS_TRUNCATE)
#define LLPP_BUILD_LANDLOCK_ABI 3
#elif defined(LANDLOCK_ACCESS_FS_REFER)
#define LLPP_BUILD_LANDLOCK_ABI 2
#elif defined(LANDLOCK_ACCESS_FS_EXECUTE)
#define LLPP_BUILD_LANDLOCK_ABI 1
#else
#error "Failed to resolve Landlock API version"
#endif
// NOLINTEND(*-macro-usage)

int main()
{
	std::cout << LLPP_BUILD_LANDLOCK_ABI;
	return 0;
}
'''

landlock_ver_res = cxx.run(landlock_ver_code, name: 'Check Landlock ABI version')

if not landlock_ver_res.compiled()
	error('Failed to get landlock ABI version')
endif

if landlock_ver_res.returncode() != 0
	error('Failed to get landlock ABI version: ', landlock_ver_res.stderr())
else
	message('Landlock ABI version: ', landlock_ver_res.stdout())
endif
