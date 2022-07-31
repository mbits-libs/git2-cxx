foreach(STRING_FILE ${STRINGS})
  list(APPEND SOURCES src/strings/${STRING_FILE}.cc include/cov/app/strings/${STRING_FILE}.hh)

    
  foreach(LANG_ID ${LANGUAGES})
    set(__lang_ID_int "intermediate/${LANG_ID}/${STRING_FILE}")
    set(__lang_ID_bin "${PROJECT_BINARY_DIR}/${SHARE_DIR}/locale/${LANG_ID}/${STRING_FILE}")
    set(__lang_ID_target "${STRING_FILE}.${LANG_ID}")

    add_custom_target(${__lang_ID_target}
      COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}" "${__lang_ID_bin}"
      DEPENDS "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}"
      COMMENT "[SHARE] ${LANG_ID}/${STRING_FILE}"
    )
    set_target_properties(${__lang_ID_target} PROPERTIES FOLDER apps/strings)
  endforeach()
endforeach()
