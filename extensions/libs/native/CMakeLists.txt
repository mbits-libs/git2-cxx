set(SOURCES
    src/platform.cc
    include/native/expat.hh
    include/native/platform.hh
    include/native/str.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(ext_native ${SOURCES})
target_include_directories(ext_native PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/native
)
target_link_libraries(ext_native PUBLIC ctre::ctre expat::expat app_main)

set_parent_scope()
