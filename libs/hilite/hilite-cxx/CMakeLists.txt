set(SOURCES
  src/cxx.cc
  src/hilite/syntax/cxx.hh
  )
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(hilite-cxx ${SOURCES})
target_link_libraries(hilite-cxx PUBLIC hilite)
set_target_properties(hilite-cxx PROPERTIES FOLDER libs/hilite)

install(
  TARGETS hilite-cxx
  COMPONENT hilite_cxx
)

install(
  DIRECTORY src/hilite/
  DESTINATION include/hilite
  COMPONENT hilite_cxx
)

cpack_add_component(hilite_cxx
  DISPLAY_NAME "libhilite-c++"
  DEPENDS hilite
  GROUP devel
  HIDDEN
)

set_parent_scope()
