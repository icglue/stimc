enable_testing ()

# Tests
set (STIMC_TESTS
	ff.tc_sanity
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

if (${SIMULATOR} STREQUAL "icarus")
    # non-standard test, only with icarus
    list (APPEND STIMC_TESTS ff_real.tc_sanity)
endif ()

string (REPLACE "icarus" "iverilog" TESTSIMULATOR "${SIMULATOR}")

foreach (test IN LISTS STIMC_TESTS)
    string (REGEX REPLACE "^([^.]+)\\.([^.]+)$" "\\1" unit     "${test}")
    string (REGEX REPLACE "^([^.]+)\\.([^.]+)$" "\\2" testcase "${test}")

    set (workdir "${CMAKE_CURRENT_SOURCE_DIR}/units/${unit}/simulation/generic/${testcase}")

    add_test (NAME "${test}.prepare"  WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} clean)
    add_test (NAME "${test}.run"      WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} runbatch)

    set_tests_properties ("${test}.run" PROPERTIES PASS_REGULAR_EXPRESSION "TBCHECK: PASSED")
    set_tests_properties ("${test}.run" PROPERTIES DEPENDS "${test}.prepare")

    set (tests "${test}.prepare" "${test}.run")

    if (${SIMULATOR} STREQUAL "icarus")
        add_test (NAME "${test}.memcheck" WORKING_DIRECTORY "${workdir}" COMMAND make SIMULATOR=${TESTSIMULATOR} memcheck)

        set_tests_properties ("${test}.memcheck" PROPERTIES DEPENDS "${test}.run")

        list (APPEND tests  "${test}.memcheck")
    endif ()

    set_tests_properties (
        ${tests}
        PROPERTIES ENVIRONMENT ICPRO_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach ()
