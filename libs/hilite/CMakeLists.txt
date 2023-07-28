cpack_add_component_group(extras
  DISPLAY_NAME "Additional libraries"
  EXPANDED
)

add_subdirectory(hilite)
add_subdirectory(hilite-cxx)
add_subdirectory(hilite-py3)
add_subdirectory(hilite-ts)
add_subdirectory(lighter)

set_parent_scope()
