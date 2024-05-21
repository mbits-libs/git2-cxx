function(fix_vs_modules TARGET)
  set(__TOOLS ${VS_MODS} ${TARGET})
  set(VS_MODS ${__TOOLS} PARENT_SCOPE)

  if (MSVC_VERSION GREATER_EQUAL 1936 AND MSVC_IDE)
    set_target_properties(${TARGET} PROPERTIES VS_USER_PROPS "${PROJECT_SOURCE_DIR}/cmake/VS17.NoModules.props")
  endif()
endfunction()

set(COV_LIBS)
set(COV_TOOLS)
set(VS_MODS)
set(LEGACY_COVERAGE)
set(COVERED_CODE)
set(NOT_COVERED_CODE)

macro(add_legacy_coverage DIRNAME TARGET)
  if (IS_DIRECTORY "${DIRNAME}/src")
    list(APPEND LEGACY_COVERAGE "${TARGET}/src")
    if (IS_DIRECTORY "${DIRNAME}/include")
      list(APPEND LEGACY_COVERAGE "${TARGET}/include")
    elseif(IS_DIRECTORY "${DIRNAME}/include2")
      list(APPEND LEGACY_COVERAGE "${TARGET}/includes")
    endif()
  else()
    list(APPEND LEGACY_COVERAGE "${TARGET}")
  endif()
endmacro()

function(add_to_coverage)
  cmake_parse_arguments(PARSE_ARGV 0 DIR "" "" "")

  set(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  cmake_path(RELATIVE_PATH TARGET_DIR BASE_DIRECTORY "${PROJECT_SOURCE_DIR}")

  foreach(DIR ${DIR_UNPARSED_ARGUMENTS})
    if (DIR STREQUAL ".")
      add_legacy_coverage("${CMAKE_CURRENT_SOURCE_DIR}" "${TARGET_DIR}")
      list(APPEND COVERED_CODE "${TARGET_DIR}")
      if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        list(APPEND NOT_COVERED_CODE "${TARGET_DIR}/tests")
      endif()
    else()
      if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
        add_legacy_coverage("${CMAKE_CURRENT_SOURCE_DIR}/${DIR}" "${TARGET_DIR}/${DIR}")
        list(APPEND COVERED_CODE "${TARGET_DIR}/${DIR}")
      endif()
      if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/tests")
        list(APPEND NOT_COVERED_CODE "${TARGET_DIR}/${DIR}/tests")
      endif()
    endif()
  endforeach()

  set(LEGACY_COVERAGE ${LEGACY_COVERAGE} PARENT_SCOPE)
  set(COVERED_CODE ${COVERED_CODE} PARENT_SCOPE)
  set(NOT_COVERED_CODE ${NOT_COVERED_CODE} PARENT_SCOPE)
endfunction()

function(add_project_to_coverage)
  add_to_coverage(.)
  set(LEGACY_COVERAGE ${LEGACY_COVERAGE} PARENT_SCOPE)
  set(COVERED_CODE ${COVERED_CODE} PARENT_SCOPE)
  set(NOT_COVERED_CODE ${NOT_COVERED_CODE} PARENT_SCOPE)
endfunction()

function(add_cov_tool TARGET)
  set(__TOOLS ${COV_TOOLS} ${TARGET})
  set(COV_TOOLS ${__TOOLS} PARENT_SCOPE)

  cmake_parse_arguments(PARSE_ARGV 1 COV "" "" "")
  add_executable(${TARGET} ${COV_UNPARSED_ARGUMENTS})
  target_compile_options(${TARGET} PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(${TARGET} PRIVATE ${ADDITIONAL_LINK_FLAGS})
  if (WIN32)
    target_link_options(${TARGET} PRIVATE /ENTRY:wmainCRTStartup)
    fix_vs_modules(${TARGET})
    set(VS_MODS ${VS_MODS} PARENT_SCOPE)
  endif()
endfunction()

function(add_cov_library TARGET)
  set(__LIBS ${COV_LIBS} ${TARGET})
  set(COV_LIBS ${__LIBS} PARENT_SCOPE)

  cmake_parse_arguments(PARSE_ARGV 1 COV "INTERFACE;NO_COVERAGE" "" "")
  if (COV_INTERFACE)
    add_library(${TARGET} INTERFACE)
    set(DEF_ACCESS INTERFACE)
  else(COV_INTERFACE)
    add_library(${TARGET} STATIC ${COV_UNPARSED_ARGUMENTS})
    target_compile_options(${TARGET} PRIVATE ${ADDITIONAL_WALL_FLAGS})
    target_link_options(${TARGET} PRIVATE ${ADDITIONAL_LINK_FLAGS})
    set(DEF_ACCESS PUBLIC)
  endif(COV_INTERFACE)
  fix_vs_modules(${TARGET})
  set(VS_MODS ${VS_MODS} PARENT_SCOPE)
  if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(${TARGET} ${DEF_ACCESS}
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}/include)
  elseif (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_include_directories(${TARGET} ${DEF_ACCESS}
      ${CMAKE_CURRENT_SOURCE_DIR}/src)
  endif()

  if (NOT COV_NO_COVERAGE)
    add_project_to_coverage()
    set(LEGACY_COVERAGE ${LEGACY_COVERAGE} PARENT_SCOPE)
    set(COVERED_CODE ${COVERED_CODE} PARENT_SCOPE)
    set(NOT_COVERED_CODE ${NOT_COVERED_CODE} PARENT_SCOPE)
  endif()
endfunction()

set(CORE_EXT)

function(setup_cov_ext COV_TOOL)
  set(__CORE ${CORE_EXT} ${COV_TOOL})
  set(CORE_EXT ${__CORE} PARENT_SCOPE)

  set_target_properties(cov-${COV_TOOL} PROPERTIES FOLDER apps/core)
  target_link_options(cov-${COV_TOOL} PRIVATE ${ADDITIONAL_LINK_FLAGS})
	target_compile_options(cov-${COV_TOOL} PRIVATE ${ADDITIONAL_WALL_FLAGS})

	set_target_properties(cov-${COV_TOOL} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CORE_DIR}"
    )

	foreach(BUILD_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
		set_target_properties(cov-${COV_TOOL} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/${CORE_DIR}"
        )
	endforeach()
endfunction()

macro(add_cov_ext COV_TOOL)
  string(REPLACE "-" "_" __COV_TOOL_SAFE_NAME "${COV_TOOL}")
  get_filename_component(LAST_DIRNAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
  if((EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.cc) AND ("${LAST_DIRNAME}" STREQUAL "cov_${__COV_TOOL_SAFE_NAME}"))
    set(${COV_TOOL}_CODE main.cc)
    add_to_coverage(.)
  elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}.cc)
    set(${COV_TOOL}_CODE cov_${__COV_TOOL_SAFE_NAME}.cc)
    add_to_coverage(.)
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}/main.cc)
    set(${COV_TOOL}_CODE cov_${__COV_TOOL_SAFE_NAME}/main.cc)
    add_to_coverage(cov_${__COV_TOOL_SAFE_NAME})
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}/cov_${__COV_TOOL_SAFE_NAME}.cc)
    set(${COV_TOOL}_CODE cov_${__COV_TOOL_SAFE_NAME}/cov_${__COV_TOOL_SAFE_NAME}.cc)
    add_to_coverage(cov_${__COV_TOOL_SAFE_NAME})
  else()
    message(FATAL_ERROR "Cannot find code for ${COV_TOOL}'s tool() function.\nTried:\n"
    " - ${CMAKE_CURRENT_SOURCE_DIR}/main.cc\n"
    " - ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}.cc\n"
    " - ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}/main.cc\n"
    " - ${CMAKE_CURRENT_SOURCE_DIR}/cov_${__COV_TOOL_SAFE_NAME}/cov_${__COV_TOOL_SAFE_NAME}.cc")
  endif()
  add_cov_tool(cov-${COV_TOOL} ${${COV_TOOL}_CODE})
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${COV_TOOL}_CODE})

	setup_cov_ext(${COV_TOOL})
	install(TARGETS cov-${COV_TOOL}
		RUNTIME DESTINATION ${CORE_DIR}
        COMPONENT tools
    )
  set(LEGACY_COVERAGE ${LEGACY_COVERAGE} PARENT_SCOPE)
  set(COVERED_CODE ${COVERED_CODE} PARENT_SCOPE)
  set(NOT_COVERED_CODE ${NOT_COVERED_CODE} PARENT_SCOPE)
endmacro()

function(print_cov_ext)
  set(__CORE ${CORE_EXT})
  string(REPLACE ";" ", " CORE_LIST "${CORE_EXT}")
  message(STATUS "Core extensions: ${CORE_LIST}")
endfunction()

function(add_cov_test TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 COV "" "" "")
  add_executable(${TARGET}-test ${COV_UNPARSED_ARGUMENTS})
  set_target_properties(${TARGET}-test PROPERTIES FOLDER tests)
  target_compile_options(${TARGET}-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(${TARGET}-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_include_directories(${TARGET}-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})
  fix_vs_modules(${TARGET}-test)
  set(VS_MODS ${VS_MODS} PARENT_SCOPE)
endfunction()

function(add_win32_icon TARGET NAME)
  set(RESNAME "${PROJECT_SOURCE_DIR}/data/icons/${NAME}")
  set(RC "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dir/icon.rc")
  set(__content "// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
// This file was autogenerated. Do not edit.

LANGUAGE 9, 1

100 ICON \"${RESNAME}\"
")

  file(GENERATE OUTPUT ${RC} CONTENT ${__content})
  target_sources(${TARGET} PRIVATE "${RC}" "${RESNAME}")
endfunction()

macro(set_parent_scope)
  set(CORE_EXT ${CORE_EXT} PARENT_SCOPE)
  set(COV_LIBS ${COV_LIBS} PARENT_SCOPE)
  set(COV_TOOLS ${COV_TOOLS} PARENT_SCOPE)
  set(VS_MODS ${VS_MODS} PARENT_SCOPE)
  set(LEGACY_COVERAGE ${LEGACY_COVERAGE} PARENT_SCOPE)
  set(COVERED_CODE ${COVERED_CODE} PARENT_SCOPE)
  set(NOT_COVERED_CODE ${NOT_COVERED_CODE} PARENT_SCOPE)
endmacro()
