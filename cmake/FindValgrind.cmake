
option(
  ENABLE_MEMCHECK_OPTION
  "If set to ON, enables automatic creation of memcheck targets"
  OFF
)

find_program(
	VALGRIND_PROGRAM
	NAMES valgrind
)

if(VALGRIND_PROGRAM)
	set(VALGRIND_PROGRAM_OPTIONS
		"--suppressions=${CMAKE_SOURCE_DIR}/tests/data/valgrind.suppression"
		"--error-exitcode=1"
		"--leak-check=full"
		"--gen-suppressions=all"
		"--quiet"
	)
endif()

find_package_handle_standard_args(
	VALGRIND DEFAULT_MSG
	VALGRIND_PROGRAM
)

function(add_valgrind_test)
        foreach(_arg ${ARGN})
                if ("VALGRIND" STREQUAL ${_arg})
                        if(ENABLE_MEMCHECK_OPTION AND VALGRIND_PROGRAM)
                                list(APPEND _vgargs ${VALGRIND_PROGRAM} ${VALGRIND_PROGRAM_OPTIONS})
                        endif()
                else()
                        list(APPEND _vgargs ${_arg})
                endif()
        endforeach()

        add_test(${_vgargs})
endfunction()

