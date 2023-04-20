configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dirs.hh.in ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh @ONLY)

set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../data")
get_filename_component(DATA_DIR "${DATA_DIR}" ABSOLUTE)

set(LNGS_DIR "${DATA_DIR}/strings")
set(TR_DIR "${DATA_DIR}/translations")

set(SOURCES
  src/args.cc
  src/path.cc
  src/tr.cc

  include/cov/app/args.hh
  include/cov/app/path.hh
  include/cov/app/tr.hh
)

set(GEN_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh
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

foreach(STRING_FILE ${STRINGS})
  list(APPEND GEN_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc
    ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh)
endforeach()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${GEN_SOURCES} PREFIX gen)

add_library(app STATIC ${SOURCES} ${GEN_SOURCES})
target_compile_options(app PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(app PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(app PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(app PUBLIC cov-api lighter mbits::liblngs mbits::args)

add_library(app_main STATIC src/main.cc)
target_compile_options(app_main PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(app_main PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_link_libraries(app_main PUBLIC app)

if (WIN32)
  target_compile_options(app_main PUBLIC /D_UNICODE /DUNICODE)
endif()

set_target_properties(app app_main PROPERTIES FOLDER libs)

foreach(STRING_FILE ${STRINGS})
  set(__LNGS_FILES "${LNGS_DIR}/${STRING_FILE}.lngs")

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/src/strings"
      COMMAND mbits::lngs
      ARGS res
          "${__LNGS_FILES}"
          --include "cov/app/strings/${STRING_FILE}.hh"
          -o "${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc"
          -w
      DEPENDS "${__LNGS_FILES}"
      VERBATIM)

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings"
      COMMAND mbits::lngs
      ARGS enums
          "${__LNGS_FILES}"
          --resource
          -o "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh"
      DEPENDS "${__LNGS_FILES}"
      VERBATIM)

  foreach(LANG_ID ${LANGUAGES})
    set(__lang_ID_int "intermediate/${LANG_ID}/${STRING_FILE}")
    set(__lang_ID_dir "${PROJECT_BINARY_DIR}/${SHARE_DIR}/locale/${LANG_ID}")
    set(__lang_ID_bin "${__lang_ID_dir}/${STRING_FILE}")

    add_custom_command(TARGET app
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${__lang_ID_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}" "${__lang_ID_bin}"
        DEPENDS "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}"
        COMMENT "[SHARE] ${LANG_ID}/${STRING_FILE}"
    )
  endforeach()
endforeach()

install(
  TARGETS app app_main
  COMPONENT libapp
)

install(
  DIRECTORY
    include/
    ${CMAKE_CURRENT_BINARY_DIR}/include/
  TYPE INCLUDE
  COMPONENT libapp
)

cpack_add_component(libapp
  DISPLAY_NAME "libapp"
  GROUP devel
)
