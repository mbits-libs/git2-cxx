configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dirs.hh.in ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh @ONLY)

set(SOURCES
  src/args.cc
  src/tr.cc

  include/cov/app/args.hh
  include/cov/app/tr.hh
)

if (UNIX)
	list(APPEND SOURCES src/posix.cc)
elseif(WIN32)
	list(APPEND SOURCES src/win32.cc)
endif()

string(REPLACE ";" ", " STRING_LIST "${STRINGS}")
message(STATUS "Strings: ${STRING_LIST}")
string(REPLACE ";" ", " LANGUAGE_LIST "${LANGUAGES}")
message(STATUS "Languages: ${LANGUAGE_LIST}")
include(strings)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh)

list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh)

add_library(app STATIC ${SOURCES})
target_compile_options(app PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(app PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(app PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(app PUBLIC cov-api lighter liblngs mbits::args)

add_library(app_main STATIC src/main.cc)
target_compile_options(app_main PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(app_main PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_link_libraries(app_main PUBLIC app)

if (WIN32)
  target_compile_options(app_main PUBLIC /D_UNICODE /DUNICODE)
endif()

set_target_properties(app app_main PROPERTIES FOLDER libs)

install(
  TARGETS app app_main
  COMPONENT libapp
)

install(
  DIRECTORY include/
  TYPE INCLUDE
  COMPONENT libapp
)

cpack_add_component(libapp
  DISPLAY_NAME "libapp"
  GROUP devel
)
