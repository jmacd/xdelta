file(REMOVE "${OUTPUT}")

execute_process(
  COMMAND "${XDELTA}" info "${FIXTURE_DIR}/change.xdelta"
  RESULT_VARIABLE info_result
  OUTPUT_VARIABLE info_output
  ERROR_VARIABLE info_error
)
if(NOT info_result EQUAL 0)
  message(FATAL_ERROR "xdelta info failed: ${info_error}")
endif()
if(NOT info_output MATCHES "patch version 1\\.1")
  message(FATAL_ERROR "fixture was not recognized as a 1.1 patch: ${info_output}")
endif()

execute_process(
  COMMAND
    "${XDELTA}" patch
    "${FIXTURE_DIR}/change.xdelta"
    "${FIXTURE_DIR}/original.txt"
    "${OUTPUT}"
  RESULT_VARIABLE patch_result
  OUTPUT_VARIABLE patch_output
  ERROR_VARIABLE patch_error
)
if(NOT patch_result EQUAL 0)
  message(FATAL_ERROR "xdelta patch failed: ${patch_output}${patch_error}")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -E compare_files
    "${FIXTURE_DIR}/updated.txt"
    "${OUTPUT}"
  RESULT_VARIABLE compare_result
)
if(NOT compare_result EQUAL 0)
  message(FATAL_ERROR "reconstructed file differs from the expected output")
endif()
