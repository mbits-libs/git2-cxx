if (WIN32)
set(EXT ".cmd")
string(APPEND POT_HINTS_CONTENT "@echo off
")
string(APPEND PO_HINTS_CONTENT "@echo off
")
endif()

string(APPEND POT_HINTS_CONTENT "cd ${PROJECT_SOURCE_DIR}/data

")
string(APPEND PO_HINTS_CONTENT "cd ${PROJECT_SOURCE_DIR}/data

")

foreach(STRING_FILE ${STRINGS})
  set(__lngs_inc "cov/app/strings/${STRING_FILE}.hh")

  list(APPEND SOURCES src/strings/${STRING_FILE}.cc include/${__lngs_inc})

  string(APPEND POT_HINTS_CONTENT "
echo [POT] data/translations/${STRING_FILE}.pot
lngs pot strings/${STRING_FILE}.lngs -o translations/${STRING_FILE}.pot -a \"Marcin Zdun <mzdun@midnightbits.com>\" -c midnightBITS -t \"${PROJECT_DESCRIPTION}\"
")

  string(APPEND PO_HINTS_CONTENT "
echo [CC] src/strings/${STRING_FILE}.cc
lngs res strings/${STRING_FILE}.lngs -o \"${CMAKE_CURRENT_SOURCE_DIR}/src/strings/${STRING_FILE}.cc\" --include \"${__lngs_inc}\" --warp
python ../tools/mark_generated.py \"${CMAKE_CURRENT_SOURCE_DIR}/src/strings/${STRING_FILE}.cc\"
echo [HH] include/${__lngs_inc}
lngs enums strings/${STRING_FILE}.lngs -o \"${CMAKE_CURRENT_SOURCE_DIR}/include/${__lngs_inc}\" --resource
python ../tools/mark_generated.py \"${CMAKE_CURRENT_SOURCE_DIR}/include/${__lngs_inc}\"
")

    
  foreach(LANG_ID ${LANGUAGES})
    set(__lang_ID_int "intermediate/${LANG_ID}/${STRING_FILE}")
    set(__lang_ID_bin "${PROJECT_BINARY_DIR}/${SHARE_DIR}/locale/${LANG_ID}/${STRING_FILE}")
    set(__lang_ID_po "translations/${STRING_FILE}-${LANG_ID}.po")
    set(__lang_ID_target "${STRING_FILE}.${LANG_ID}")

    string(APPEND PO_HINTS_CONTENT "echo [INT] data/intermediate/${LANG_ID}/${STRING_FILE}
lngs make strings/${STRING_FILE}.lngs -o ${__lang_ID_int} -m ${__lang_ID_po} -l translations/llcc.txt
")

    add_custom_target(${__lang_ID_target}
      COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}" "${__lang_ID_bin}"
      DEPENDS "${PROJECT_SOURCE_DIR}/data/${__lang_ID_int}"
      COMMENT "[SHARE] ${LANG_ID}/${STRING_FILE}"
    )
    set_target_properties(${__lang_ID_target} PROPERTIES FOLDER apps/strings)
  endforeach()
endforeach()

file(GENERATE
    OUTPUT ${PROJECT_BINARY_DIR}/lngs-pot${EXT}
    CONTENT ${POT_HINTS_CONTENT}
)

file(GENERATE
    OUTPUT ${PROJECT_BINARY_DIR}/lngs-make${EXT}
    CONTENT ${PO_HINTS_CONTENT}
)
