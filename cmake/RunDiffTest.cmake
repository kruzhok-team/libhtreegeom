# Runs a test program and compares its output with the expected file.
# Arguments: -DTEST_PROG=<binary> -DTEST_OUTPUT=<stdout capture>
#            [-DTEST_EXPECTED=<golden file>]

execute_process(COMMAND ${TEST_PROG} OUTPUT_FILE ${TEST_OUTPUT}
                RESULT_VARIABLE run_result)
if(run_result)
  message(FATAL_ERROR "test run failed: ${TEST_PROG} exited with ${run_result}")
endif()

if(TEST_EXPECTED)
  execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
                          ${TEST_OUTPUT} ${TEST_EXPECTED}
                  RESULT_VARIABLE diff_result)
  if(diff_result)
    message(FATAL_ERROR
            "output ${TEST_OUTPUT} didn't match the pattern ${TEST_EXPECTED}")
  endif()
endif()
