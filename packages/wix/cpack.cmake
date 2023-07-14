set(CPACK_PACKAGE_NAME Cov)

list(REMOVE_ITEM CPACK_COMPONENTS_ALL
    libcov
    librt
    libapp
    hilite
    hilite_cxx
    lighter
)

foreach(COMP ${CPACK_COMPONENTS_ALL})
    string(TOUPPER ${COMP} COMP_UPPER)
    list(REMOVE_AT CPACK_COMPONENT_${COMP_UPPER}_GROUP 0)
endforeach()
