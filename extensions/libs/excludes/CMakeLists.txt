set(SOURCES
    src/excludes.cc
    src/strip.cc
    include/strip/excludes.hh
    include/strip/strip.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(excludes ${SOURCES})
target_link_libraries(excludes PUBLIC ext_native json cov-api)

set_parent_scope()

