set(SOURCES
  parser.cc
  parser.hh
  stage.cc
  stage.hh
)
add_cov_ext(export)
target_link_libraries(cov-export PRIVATE app_main web json)
target_sources(cov-export PRIVATE ${SOURCES})

set_parent_scope()
