set(STYLESHEETS
    html/style/style.scss
    html/style/_container.scss
    html/style/_heading.scss
    html/style/_reset.scss
    html/style/_stats_table.scss
)

set(HTML_DIR "${PROJECT_BINARY_DIR}/${SHARE_DIR}/site/html")
set(RES_DIR "${PROJECT_BINARY_DIR}/${SHARE_DIR}/site/res")
set(CXX_FILT_DIR "${PROJECT_BINARY_DIR}/${SHARE_DIR}/c++filt")

set(TGT_SCRIPT
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "c++filt" "${CXX_FILT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${HTML_DIR}/style"
)

foreach(DIR img js)
    if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/data/html/${DIR}")
        list(APPEND TGT_SCRIPT
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "html/${DIR}" "${HTML_DIR}/${DIR}"
        )
    endif()
endforeach()

list(APPEND TGT_SCRIPT
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "html/res" "${RES_DIR}"
)

foreach(FILE ${STYLESHEETS})
    get_filename_component(FNAME "${FILE}" NAME_WE)
    if (FNAME MATCHES "^_")
        list(APPEND TGT_SOURCES "${FILE}")
    else()
        list(APPEND TGT_DEPS "${FILE}")
        list(APPEND TGT_SCRIPT
            COMMAND ${SassC_EXECUTABLE}
            --style expanded
            "${FILE}"
            "${HTML_DIR}/style/${FNAME}.css"
        )
    endif()
endforeach()

add_custom_target(styles ALL
    ${TGT_SCRIPT}
    DEPENDS ${TGT_DEPS}
    COMMENT "Building site files"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/data")
