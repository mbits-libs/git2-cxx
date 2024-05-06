set(SOURCES
  src/cvg_info.cc
  src/line_printer.cc
  src/report_stats.cc
  src/column_selectors.cc
  src/column_selectors.hh

  include/cov/core/cvg_info.hh
  include/cov/core/line_printer.hh
  include/cov/core/report_stats.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(cov-core ${SOURCES})
target_link_libraries(cov-core PUBLIC app)

set_target_properties(cov-core PROPERTIES FOLDER libs)

set_parent_scope()
