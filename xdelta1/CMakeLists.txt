cmake_minimum_required(VERSION 3.20)

project(xdelta VERSION 1.1.5 LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gmodule-2.0)
find_package(ZLIB REQUIRED)

configure_file(cmake/config.h.in config.h)

set(EDSIO_SOURCES
  libedsio/base64.c
  libedsio/default.c
  libedsio/edsio.c
  libedsio/edsio_edsio.c
  libedsio/fh.c
  libedsio/generic.c
  libedsio/library.c
  libedsio/maketime.c
  libedsio/md5c.c
  libedsio/partime.c
  libedsio/sha.c
  libedsio/simple.c
)

add_library(edsio STATIC ${EDSIO_SOURCES})
target_include_directories(edsio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libedsio>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/xdelta>
)
target_link_libraries(edsio PUBLIC PkgConfig::GLIB)

add_library(xdelta_lib xdelta.c xdapply.c xd_edsio.c)
set_target_properties(xdelta_lib PROPERTIES OUTPUT_NAME xdelta)
target_include_directories(xdelta_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libedsio>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/xdelta>
)
target_link_libraries(xdelta_lib PUBLIC edsio)

add_executable(xdelta xdmain.c getopt.c getopt1.c)
target_link_libraries(xdelta PRIVATE xdelta_lib ZLIB::ZLIB)

install(TARGETS xdelta xdelta_lib edsio
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES xdelta.h xd_edsio.h libedsio/edsio.h libedsio/edsio_edsio.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xdelta
)
