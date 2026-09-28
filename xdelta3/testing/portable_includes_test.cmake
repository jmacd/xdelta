cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED HEADER)
  message(FATAL_ERROR "HEADER must name the public header to check")
endif()

set(PORTABLE_HEADERS
  errno.h inttypes.h stdarg.h stddef.h stdio.h stdlib.h string.h)
set(WINDOWS_HEADERS windows.h stdint.h)

file(STRINGS "${HEADER}" HEADER_LINES)

set(CONDITION_DEPTH 0)
set(WINDOWS_GUARD_DEPTH -1)
set(IN_WINDOWS_BRANCH FALSE)
set(LINE_NUMBER 0)

foreach(LINE IN LISTS HEADER_LINES)
  math(EXPR LINE_NUMBER "${LINE_NUMBER} + 1")
  string(STRIP "${LINE}" STRIPPED_LINE)

  if(STRIPPED_LINE MATCHES "^#[ \t]*ifndef[ \t]+_WIN32([ \t]|$)")
    math(EXPR CONDITION_DEPTH "${CONDITION_DEPTH} + 1")
    set(WINDOWS_GUARD_DEPTH "${CONDITION_DEPTH}")
    set(IN_WINDOWS_BRANCH FALSE)
  elseif(STRIPPED_LINE MATCHES "^#[ \t]*(if|ifdef|ifndef)([ \t(]|$)")
    math(EXPR CONDITION_DEPTH "${CONDITION_DEPTH} + 1")
  elseif(STRIPPED_LINE MATCHES "^#[ \t]*(else|elif)([ \t(]|$)")
    if(CONDITION_DEPTH EQUAL WINDOWS_GUARD_DEPTH)
      if(STRIPPED_LINE MATCHES "^#[ \t]*else([ \t]|$)")
        set(IN_WINDOWS_BRANCH TRUE)
      else()
        set(IN_WINDOWS_BRANCH FALSE)
      endif()
    endif()
  elseif(STRIPPED_LINE MATCHES "^#[ \t]*endif([ \t]|$)")
    if(CONDITION_DEPTH EQUAL WINDOWS_GUARD_DEPTH)
      set(WINDOWS_GUARD_DEPTH -1)
      set(IN_WINDOWS_BRANCH FALSE)
    endif()
    math(EXPR CONDITION_DEPTH "${CONDITION_DEPTH} - 1")
  elseif(STRIPPED_LINE MATCHES "^#[ \t]*include[ \t]*<([^>]+)>")
    set(INCLUDE "${CMAKE_MATCH_1}")
    if(INCLUDE IN_LIST PORTABLE_HEADERS)
      continue()
    endif()
    if(IN_WINDOWS_BRANCH AND INCLUDE IN_LIST WINDOWS_HEADERS)
      continue()
    endif()
    message(FATAL_ERROR
      "${HEADER}:${LINE_NUMBER}: <${INCLUDE}> is not an approved portable "
      "header or "
      "an approved header inside the _WIN32 branch")
  endif()
endforeach()

message(STATUS "${HEADER} uses only approved portable includes")
