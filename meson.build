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

subdir('meson')

conf_data = configuration_data({
	'landlock_version': landlock_ver_res.stdout(),
})

public_include = include_directories('./include')

subdir('src')

if get_option('test')
	subdir('test')
endif

# vi: noexpandtab
