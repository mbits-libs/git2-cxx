set(SOURCES
  src/components.cc
  src/link_service.cc
  src/mstch_cache.cc

  include/web/components.hh
  include/web/link_service.hh
  include/web/mstch_cache.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(web ${SOURCES})
target_link_libraries(web PUBLIC ext_platform mstch c++filt)

set_target_properties(web PROPERTIES FOLDER libs)

set_parent_scope()
