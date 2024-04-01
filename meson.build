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

if not cxx.has_header('linux/landlock.h')
	error('Your system appears not to have Landlock support!')
endif

public_include = include_directories('./include')

subdir('src')

if get_option('test')
	subdir('test')
endif

# vi: noexpandtab
