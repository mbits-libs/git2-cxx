add_subdirectory(libs)
add_subdirectory(cov_export)

set(NATIVE_FILTERS
    strip-excludes
)

set(NATIVE_FILTERS ${NATIVE_FILTERS} PARENT_SCOPE)

add_to_coverage(filters)

foreach(NAME ${NATIVE_FILTERS})
    string(REPLACE "-" "_" SAFE_NAME "${NAME}")
    add_cov_tool(${NAME} filters/${SAFE_NAME}.cc)
    target_include_directories(${NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/filters/${SAFE_NAME}")
    add_win32_icon(${NAME} "plugin.ico")
    set_target_properties(${NAME} PROPERTIES FOLDER apps)

    set_target_properties(${NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${SHARE_DIR}/filters"
      )

    foreach(BUILD_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
      set_target_properties(${NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/${SHARE_DIR}/filters"
          )
    endforeach()
endforeach()

target_sources(strip-excludes PRIVATE
  filters/strip_excludes/parser.hh
  filters/strip_excludes/parser.cc
)
target_link_libraries(strip-excludes PRIVATE excludes ext_platform)

set_parent_scope()
