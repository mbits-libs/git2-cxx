add_subdirectory(libs)

set(NATIVE_FILTERS
)

set(NATIVE_FILTERS ${NATIVE_FILTERS} PARENT_SCOPE)

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

set_parent_scope()
