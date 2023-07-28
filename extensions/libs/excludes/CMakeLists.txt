set(SOURCES
    src/excludes.cc
    src/excludes.hh
    src/parser.cc
    src/parser.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(excludes ${SOURCES})
target_link_libraries(excludes PUBLIC ext_native)

set_parent_scope()
