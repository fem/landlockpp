catch_dep = dependency('catch2-with-main', required: true)

tests = files([
	'ActionTypeTest.cpp',
	'RuleTest.cpp',
])

test_runner = executable(
	'tests',
	[
		tests,
	],
	include_directories: [
		public_include,
		src_include,
	],
	link_with: [
		liblandlockpp,
	],
	dependencies: [
		catch_dep
	],
)

test_tgt = run_target(
	'check',
	command: [
		'/bin/sh',
		'-c',
		'"${MESON_BUILD_ROOT}/test/tests"',
	],
)
