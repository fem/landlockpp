src_include = include_directories('.')

# config.h generation is in subdir to make sure the build-time directory structure
# matches the installed directory structure
subdir('ll')

liblandlockpp = library(
	'landlockpp',
	[
		'ActionType.cpp',
		'Rule.cpp',
		'Ruleset.cpp',
	],
	include_directories: [
		src_include,
		public_include,
	],
)
