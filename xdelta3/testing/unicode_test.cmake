if(NOT DEFINED XDELTA OR NOT DEFINED TEST_DIR)
  message(FATAL_ERROR "XDELTA and TEST_DIR are required")
endif()

set(unicode_name "Restauração-漢字")
set(source_dir "${TEST_DIR}/${unicode_name}-source")
set(target_dir "${TEST_DIR}/${unicode_name}-target")
set(patch_dir "${TEST_DIR}/${unicode_name}-patch")
set(source "${source_dir}/${unicode_name}.txt")
set(target "${target_dir}/${unicode_name}.txt")
set(patch "${patch_dir}/${unicode_name}.vcdiff")
set(decoded "${target_dir}/${unicode_name}-decoded.txt")

file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${source_dir}" "${target_dir}" "${patch_dir}")
file(WRITE "${source}" "before\n")
file(WRITE "${target}" "before\nafter\n")

execute_process(
  COMMAND "${XDELTA}" -e -s "${source}" "${target}" "${patch}"
  RESULT_VARIABLE encode_result
  ERROR_VARIABLE encode_error)
if(NOT encode_result EQUAL 0)
  set(failure "Unicode encode failed (${encode_result}): ${encode_error}")
endif()

if(NOT DEFINED failure)
  execute_process(
    COMMAND "${XDELTA}" -d -s "${source}" "${patch}" "${decoded}"
    RESULT_VARIABLE decode_result
    ERROR_VARIABLE decode_error)
  if(NOT decode_result EQUAL 0)
    set(failure "Unicode decode failed (${decode_result}): ${decode_error}")
  endif()
endif()

if(NOT DEFINED failure)
  file(SHA256 "${target}" expected)
  file(SHA256 "${decoded}" actual)
  if(NOT actual STREQUAL expected)
    set(failure "Decoded Unicode-path file does not match the target")
  endif()
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
if(DEFINED failure)
  message(FATAL_ERROR "${failure}")
endif()
