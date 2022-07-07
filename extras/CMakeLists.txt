cpack_add_component_group(extras
  DISPLAY_NAME "Additional libraries"
  EXPANDED
)

add_subdirectory(hilite_cxx)
add_subdirectory(hilite_py3)
add_subdirectory(hilite_ts)
add_subdirectory(highlighter)
