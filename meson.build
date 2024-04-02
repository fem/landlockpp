project(
	'landlockpp',
	[
		'cpp'
	],
	version: '0.1_alpha1',
	default_options: [
		'warning_level=3',
		'cpp_std=c++17'
	]
)

cxx = meson.get_compiler('cpp')

required_headers = [
	'linux/landlock.h',
	'sys/syscall.h',
	'unistd.h',
]

foreach h : required_headers
	if not cxx.has_header(h)
		error('Your system is missing required headers for Landlock support!')
	endif
endforeach

landlock_ver_code = '''
#include <iostream>
#include <cstring>
extern "C" {
#include <linux/landlock.h>
#include <sys/syscall.h>
#include <unistd.h>
}

int main()
{
	const int res = (int) syscall(SYS_landlock_create_ruleset, nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION);
	if (res < 0) {
		std::clog << strerror(errno);
		return 1;
	}

	std::cout << res;
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

conf_data = configuration_data({
	'landlock_version': landlock_ver_res.stdout(),
})

public_include = include_directories('./include')

subdir('src')

if get_option('test')
	subdir('test')
endif

# vi: noexpandtab
