cpack_add_component_group(devel
  DISPLAY_NAME "Libraries and headers"
  EXPANDED
)

add_subdirectory(helpers)
add_subdirectory(hilite)
add_subdirectory(cov-api)
add_subdirectory(app)
add_subdirectory(cov-rt)
