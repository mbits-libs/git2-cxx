set(SOURCES
  src/cell/ascii.cc
  src/cell/character.cc
  src/cell/context.cc
  src/cell/operators.cc
  src/cell/parser.cc
  src/cell/repeat_operators.cc
  src/cell/string.cc
  src/cell/special.cc
  src/cell/tokens.cc
  src/hilite.cc
  src/none.cc
  include/cell/ascii.hh
  include/cell/character.hh
  include/cell/context.hh
  include/cell/operators.hh
  include/cell/parser.hh
  include/cell/repeat_operators.hh
  include/cell/string.hh
  include/cell/special.hh
  include/cell/tokens.hh
  include/hilite/hilite.hh
  include/hilite/none.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(hilite STATIC ${SOURCES})
target_compile_options(hilite PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(hilite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(hilite PROPERTIES FOLDER libs)

install(
  TARGETS hilite
  COMPONENT hilite
)

install(
  DIRECTORY include/
  TYPE INCLUDE
  COMPONENT hilite
)

cpack_add_component(hilite
  DISPLAY_NAME "libhilite"
  GROUP devel
)
