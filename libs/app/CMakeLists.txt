configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dirs.hh.in ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh @ONLY)

set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../data")
get_filename_component(DATA_DIR "${DATA_DIR}" ABSOLUTE)

set(LNGS_DIR "${DATA_DIR}/strings")
set(TR_DIR "${DATA_DIR}/translations")
set(LOCALE_DIR "${PROJECT_BINARY_DIR}/${SHARE_DIR}/locale")

set(SOURCES
  src/args.cc
  src/path.cc
  src/tr.cc

  include/cov/app/args.hh
  include/cov/app/path.hh
  include/cov/app/tr.hh
)

if (UNIX)
	list(APPEND SOURCES src/posix.cc)
elseif(WIN32)
	list(APPEND SOURCES src/win32.cc)
endif()

set(GEN_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/dirs.hh
)
set(DATA_SOURCES)
set(SHARE_SOURCES)

string(REPLACE ";" ", " STRING_LIST "${STRINGS}")
message(STATUS "Strings: ${STRING_LIST}")
string(REPLACE ";" ", " LANGUAGE_LIST "${LANGUAGES}")
message(STATUS "Languages: ${LANGUAGE_LIST}")

foreach(STRING_FILE ${STRINGS})
  list(APPEND GEN_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc
    ${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh)

  list(APPEND DATA_SOURCES "${TR_DIR}/${STRING_FILE}.pot" "${LNGS_DIR}/${STRING_FILE}.lngs")

  foreach(LANG ${LANGUAGES})
    set(__po "${TR_DIR}/${STRING_FILE}/${LANG}.po")
    set(__lng "${LOCALE_DIR}/${LANG}/${STRING_FILE}")
    if(EXISTS ${__po})
      list(APPEND DATA_SOURCES ${__po})
      list(APPEND SHARE_SOURCES ${__lng})
    endif()
  endforeach()
endforeach()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${GEN_SOURCES} PREFIX gen)
source_group(TREE "${PROJECT_BINARY_DIR}/${SHARE_DIR}" FILES ${SHARE_SOURCES} PREFIX gen/share)
source_group(TREE ${DATA_DIR} FILES ${DATA_SOURCES} PREFIX data)

add_cov_library(app ${SOURCES} ${GEN_SOURCES} ${SHARE_SOURCES} ${DATA_SOURCES})
target_link_libraries(app PUBLIC cov-api lighter mbits::liblngs mbits::args)

add_library(app_main STATIC src/main.cc)
target_compile_options(app_main PUBLIC ${ADDITIONAL_WALL_FLAGS})
target_link_options(app_main PUBLIC ${ADDITIONAL_LINK_FLAGS})
target_link_libraries(app_main PUBLIC app)

if (WIN32)
  target_compile_options(app_main PUBLIC /D_UNICODE /DUNICODE)
endif()

set_target_properties(app app_main PROPERTIES FOLDER libs)

foreach(STRING_FILE ${STRINGS})
  set(__LNGS_FILE "${LNGS_DIR}/${STRING_FILE}.lngs")

  add_custom_command(OUTPUT "${TR_DIR}/${STRING_FILE}.pot"
    COMMAND mbits::lngs
    ARGS pot "${__LNGS_FILE}" -o "${TR_DIR}/${STRING_FILE}.pot" -a "marcin.zdun@midnightbits.com"
    DEPENDS "${__LNGS_FILE}"
    COMMENT "[POT] ${STRING_FILE}"
    VERBATIM)

  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc"
      "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh"

    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/src/strings"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings"
    COMMAND mbits::lngs
    ARGS res
      "${__LNGS_FILE}"
      --include "cov/app/strings/${STRING_FILE}.hh"
      -o "${CMAKE_CURRENT_BINARY_DIR}/src/strings/${STRING_FILE}.cc"
      -w
    COMMAND mbits::lngs
    ARGS enums
      "${__LNGS_FILE}"
      --resource
      -o "${CMAKE_CURRENT_BINARY_DIR}/include/cov/app/strings/${STRING_FILE}.hh"
    DEPENDS "${__LNGS_FILE}"
    COMMENT "[ENUM+RES] ${STRING_FILE}"
    VERBATIM)

  foreach(LANG ${LANGUAGES})
    set(__po "${TR_DIR}/${STRING_FILE}/${LANG}.po")
    set(__lng "${LOCALE_DIR}/${LANG}/${STRING_FILE}")
    if (EXISTS ${__po})
      add_custom_command(OUTPUT "${__lng}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LOCALE_DIR}/${LANG}"
        COMMAND mbits::lngs
        ARGS make
        "${__LNGS_FILE}"
        -o "${__lng}"
        --msgs "${__po}"
        --lang "${TR_DIR}/llcc.txt"
        DEPENDS ${__po} "${__LNGS_FILE}"
        COMMENT "[SHARE] ${LANG}/${STRING_FILE}"
        VERBATIM)
    else()
      message(STATUS "waiting for ${__po}")
    endif()
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

set_parent_scope()
