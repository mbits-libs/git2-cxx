cpack_add_component_group(devel
  DISPLAY_NAME "Libraries and headers"
  EXPANDED
)

add_subdirectory(git2-c++)
add_subdirectory(cov)
