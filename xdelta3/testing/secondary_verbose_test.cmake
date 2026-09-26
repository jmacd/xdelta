if(NOT DEFINED XDELTA OR NOT DEFINED TEST_DIR)
  message(FATAL_ERROR "XDELTA and TEST_DIR are required")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${TEST_DIR}")
file(WRITE "${TEST_DIR}/source" "source content\n")
file(WRITE "${TEST_DIR}/target" "target content\n")

execute_process(
  COMMAND "${XDELTA}" -e -v -S none
    -s "${TEST_DIR}/source" "${TEST_DIR}/target" "${TEST_DIR}/delta"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
  message(FATAL_ERROR
    "xdelta3 encode failed (${result})\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

if(NOT stderr MATCHES "secondary compression: none")
  message(FATAL_ERROR
    "xdelta3 did not report disabled secondary compression\nstderr:\n${stderr}")
endif()
