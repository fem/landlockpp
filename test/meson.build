threads_dep = dependency('threads', required: true)

test_deps = [threads_dep]
test_conf_data = {}
add_test_main = false

catch_dep = dependency('catch2-with-main', required: false)

if catch_dep.found()
	test_deps += catch_dep
else
	catch_dep = dependency('catch2', required: true)
	test_deps += catch_dep
	names = [
		'Catch2Main',
		'Catch2WithMain',
	]

	lib_dep_found = false
	foreach n : names
		lib_dep = cxx.find_library(
			n,
			dirs: [
				catch_dep.get_variable('libdir')
			],
			required: false
		)
		if lib_dep.found()
			lib_dep_found = true
			test_deps += lib_dep
			break
		endif
	endforeach

	if not lib_dep_found
		message('No Catch-provided main() found. Trying to add test_main.cpp source to provide main()')
		add_test_main = true
	endif
endif

catch_has_split_hdr = cxx.has_header(
	'catch2/catch_test_macros.hpp',
	required: false
)
if catch_has_split_hdr
	test_conf_data += {
		'HAVE_CATCH_SPLIT_HEADER': true,
	}
else
	cxx.has_header(
		'catch2/catch.hpp',
		required: true
	)
	test_conf_data += {
		'HAVE_CATCH_SPLIT_HEADER': false
	}
endif

test_header = configure_file(
	input: 'test.hpp.in',
	output: 'test.hpp',
	configuration: test_conf_data,
	install: false,
)

tests = files([
	'CodedTypeTest.cpp',
	'RuleTest.cpp',
	'RulesetTest.cpp',
	'typingTest.cpp',
])

if add_test_main
	tests += files([
		'test_main.cpp'
	])
endif

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
	dependencies: test_deps,
)

test_tgt = run_target(
	'check',
	command: [
		'/bin/sh',
		'-c',
		'"${MESON_BUILD_ROOT}/test/tests"',
	],
)

test('tests', test_runner)
