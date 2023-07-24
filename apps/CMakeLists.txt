
set(BUILTINS
    branch
    checkout
    config
    init
    log
    module
    report
    reset
    show
    tag
)

set(REPORT_FILTERS
    cobertura
    coveralls
)

set(SOURCES
	  cov/cov.cc
)

foreach(NAME ${BUILTINS})
    string(REPLACE "-" "_" SAFE_NAME "${NAME}")
    list(APPEND SOURCES builtins/builtin_${SAFE_NAME}.cc)
endforeach()

foreach(NAME ${REPORT_FILTERS})
    list(APPEND SOURCES filters/${NAME}.py)
endforeach()

string(REPLACE ";" ", " BUILTIN_LIST "${BUILTINS}")
message(STATUS "Builtins: ${BUILTIN_LIST}")
include(builtins)

string(REPLACE ";" ", " REPORT_FILTER_LIST "${REPORT_FILTERS}")
message(STATUS "Filters: ${REPORT_FILTER_LIST}")

if (WIN32)
    list(APPEND SOURCES
        win32/cov.rc
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${CMAKE_CURRENT_BINARY_DIR}/gen/cov/builtins.hh ${CMAKE_CURRENT_BINARY_DIR}/gen/cov/versioninfo.rc)

list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/gen/cov/builtins.hh)

if (WIN32)
    foreach(LANG ${LANGUAGES})
        list(APPEND PO_FILES "${CMAKE_SOURCE_DIR}/data/translations/cov/${LANG}.po")
    endforeach()

    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/cov/versioninfo.rc"
        COMMAND ${Python3_EXECUTABLE}
        ARGS
            "${CMAKE_SOURCE_DIR}/tools/win32/winrc.py"
            "${CMAKE_CURRENT_BINARY_DIR}/gen/cov"
            "${CMAKE_CURRENT_SOURCE_DIR}/win32/versioninfo.h"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/win32/winrc.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/win32/versioninfo.h"
            ${PO_FILES}
        VERBATIM
        COMMENT "Generating VERSIONINFO for the executable."
    )

    list(APPEND SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/gen/cov/versioninfo.rc"
        "${CMAKE_SOURCE_DIR}/data/icons/win32.ico"
    )
endif()

add_cov_tool(cov ${SOURCES})
target_compile_options(cov PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(cov PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(cov PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/gen
)
target_link_libraries(cov PRIVATE cov-rt)
set_target_properties(cov PROPERTIES FOLDER apps)

########################################################

foreach(NAME ${REPORT_FILTERS})
    set(__input ${CMAKE_CURRENT_SOURCE_DIR}/filters/${NAME}.py)
    set(__output ${CMAKE_BINARY_DIR}/${SHARE_DIR}/filters/${NAME})

    add_custom_command(TARGET cov
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/${SHARE_DIR}/filters"
        COMMAND "${CMAKE_COMMAND}" -E copy "${__input}" "${__output}"
        COMMENT "[FILTER] ${NAME}"
    )
endforeach()

########################################################

# add_cov_ext(serve)

print_cov_ext()

########################################################

if (COV_TESTING)
    enable_testing()

    add_cov_tool(cov-echo tests/cov-echo.cc)
    target_compile_options(cov-echo PRIVATE ${ADDITIONAL_WALL_FLAGS})
    target_link_options(cov-echo PRIVATE ${ADDITIONAL_LINK_FLAGS})
    target_link_libraries(cov-echo PRIVATE fmt::fmt mbits::args)
    set_target_properties(cov-echo PROPERTIES FOLDER tests/spawn)

    add_cov_tool(cov-pwd tests/cov-pwd.cc)
    target_compile_options(cov-pwd PRIVATE ${ADDITIONAL_WALL_FLAGS})
    target_link_options(cov-pwd PRIVATE ${ADDITIONAL_LINK_FLAGS})
    target_link_libraries(cov-pwd PRIVATE fmt::fmt mbits::args)
    set_target_properties(cov-pwd PROPERTIES FOLDER tests/spawn)

    set_target_properties(cov-echo PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/elsewhere/${CORE_DIR}"
      )
    set_target_properties(cov-pwd PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dont-copy"
      )

    foreach(BUILD_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
      set_target_properties(cov-echo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/elsewhere/${CORE_DIR}"
          )
      set_target_properties(cov-pwd PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/dont-copy"
          )
    endforeach()

    foreach(TEST_SET
        001-cov
        002-init
        003-config-A 004-config-B 005-config-C
        006-module-A 007-module-B
        008-report-A 009-report-B
        010-log
        011-refs
        012-checkout
        013-reset
        014-show
    )
        add_test(
            NAME cov-exec--${TEST_SET}
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_runner.py"
                --bin "${PROJECT_BINARY_DIR}"
                --tests "main-set/${TEST_SET}"
                --version "${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}${PROJECT_VERSION_BUILD_META}"
                )
    endforeach()

    if (NOT cov_COVERALLS)
        add_test(
            NAME cov-exec-pl
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_runner.py"
                --bin "${PROJECT_BINARY_DIR}"
                --tests "messages-pl"
                --version "${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}${PROJECT_VERSION_BUILD_META}"
                )
    endif()

  if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
    add_dependencies(cov_coveralls_test
      cov-echo
    )
  endif()
endif()

install(TARGETS cov
    RUNTIME DESTINATION ${BINARY_DIR}
    COMPONENT main_exec
)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/share/
    DESTINATION share/
    USE_SOURCE_PERMISSIONS
    COMPONENT main_exec
)

cpack_add_component(main_exec
    DISPLAY_NAME "Main executable"
    GROUP apps
)

cpack_add_component(tools
    DISPLAY_NAME "Core tools"
    GROUP apps
)

cpack_add_component_group(apps
    DISPLAY_NAME "Executables"
    EXPANDED
)
