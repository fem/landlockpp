src_include = include_directories('.')

# config.h generation is in subdir to make sure the build-time directory structure
# matches the installed directory structure
subdir('ll')

liblandlockpp = library(
	'landlockpp',
	[
		'Rule.cpp',
		'Ruleset.cpp',
	],
	include_directories: [
		src_include,
		public_include,
	],
	install: true,
	version: '0.2.0',
	cpp_args: [
		'-D_LLPP_EXPORTS',
	],
	gnu_symbol_visibility: 'hidden',
)

pkg_config = import('pkgconfig')
pkg_config.generate(
	liblandlockpp,
	libraries: [
		liblandlockpp,
	],
	subdirs: ['.'],
)
