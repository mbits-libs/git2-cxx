set(SOURCES
    src/platform.cc
    include/native/platform.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(ext_platform ${SOURCES})
target_include_directories(ext_platform PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/native
)
target_link_libraries(ext_platform PUBLIC app_main)

set_parent_scope()
