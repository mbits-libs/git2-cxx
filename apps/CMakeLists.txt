
set(BUILTINS
    init
)

set(SOURCES
	cov/cov.cc
)

foreach(NAME ${BUILTINS})
    string(REPLACE "-" "_" SAFE_NAME "${NAME}")
    list(APPEND SOURCES builtins/builtin_${SAFE_NAME}.cc)
endforeach()

string(REPLACE ";" ", " BUILTIN_LIST "${BUILTINS}")
message(STATUS "Builtins: ${BUILTIN_LIST}")
include(builtins)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${CMAKE_CURRENT_BINARY_DIR}/cov/builtins.hh)

list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/cov/builtins.hh)

add_cov_tool(cov ${SOURCES})
target_compile_options(cov PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(cov PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(cov PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(cov PRIVATE cov-rt)
set_target_properties(cov PROPERTIES FOLDER apps)

########################################################

# add_cov_ext(serve)

print_cov_ext()

########################################################

add_custom_target(copy-langs ALL)
add_dependencies(cov copy-langs)
set_target_properties(copy-langs PROPERTIES FOLDER apps/strings)

foreach(STRING_FILE ${STRINGS})
    foreach(LANG_ID ${LANGUAGES})
        add_dependencies(copy-langs "${STRING_FILE}.${LANG_ID}")
    endforeach()
endforeach()

########################################################

if (COV_TESTING)
    enable_testing()

    add_cov_tool(cov-echo tests/cov-echo.cc)
    target_compile_options(cov-echo PRIVATE ${ADDITIONAL_WALL_FLAGS})
    target_link_options(cov-echo PRIVATE ${ADDITIONAL_LINK_FLAGS})
    target_link_libraries(cov-echo PRIVATE fmt::fmt mbits::args)
    set_target_properties(cov-echo PROPERTIES FOLDER tests/spawn)

	set_target_properties(cov-echo PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/elsewhere/${CORE_DIR}"
    )

	foreach(BUILD_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
		set_target_properties(cov-echo PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/elsewhere/${CORE_DIR}"
        )
	endforeach()

    add_test(
        NAME cov-exec
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_driver.py"
            --target "$<TARGET_FILE:cov>"
            --tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/main-set"
            --data-dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
            --version "${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}${PROJECT_VERSION_BUILD_META}"
            --install "${CMAKE_CURRENT_SOURCE_DIR}/tests/copy"
            --install-with "$<TARGET_FILE:cov-echo>"
            )

    add_test(
        NAME cov-exec-pl
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_driver.py"
            --target "$<TARGET_FILE:cov>"
            --tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/messages-pl"
            --data-dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
            --version "${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}${PROJECT_VERSION_BUILD_META}"
            --install "${CMAKE_CURRENT_SOURCE_DIR}/tests/copy"
            --install-with "$<TARGET_FILE:cov-echo>"
            )

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
