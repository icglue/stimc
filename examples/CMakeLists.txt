
enable_testing ()

# Tests
set (tests
	ff.tc_sanity
    ff_real.tc_sanity
    dummy.tc_sanity
    dummy.tc_events_1
    dummy.tc_events_2
    dummy.tc_events_3
    dummy.tc_cleanup_simple
    dummy.tc_cleanup_stack
    dummy.tc_threads
    dummy_c.tc_sanity
    iotest.tc_sanity
)

string (REPLACE "icarus" "iverilog" TESTSIMULATOR "${SIMULATOR}")

foreach (test IN LISTS tests)
    string (REGEX REPLACE "^([^.]+)\\.([^.]+)$" "\\1" unit     "${test}")
    string (REGEX REPLACE "^([^.]+)\\.([^.]+)$" "\\2" testcase "${test}")

    set (workdir "${CMAKE_CURRENT_SOURCE_DIR}/units/${unit}/simulation/generic/${testcase}")

    add_test (NAME "${test}.prepare"  WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} clean)
    add_test (NAME "${test}.run"      WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} runbatch)
    add_test (NAME "${test}.memcheck" WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} memcheck)

    set_tests_properties (
        "${test}.prepare" "${test}.run" "${test}.memcheck"
        PROPERTIES ENVIRONMENT ICPRO_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_tests_properties ("${test}.run" PROPERTIES PASS_REGULAR_EXPRESSION "TBCHECK: PASSED")

    set_tests_properties ("${test}.run" PROPERTIES DEPENDS "${test}.prepare")
    set_tests_properties ("${test}.memcheck" PROPERTIES DEPENDS "${test}.run")
endforeach ()


