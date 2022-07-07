set(SOURCES
  src/cxx.cc
  src/hilite/cxx.hh
  )
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(hilite_cxx STATIC ${SOURCES})
target_compile_options(hilite_cxx PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(hilite_cxx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(hilite_cxx PUBLIC hilite)
set_target_properties(hilite_cxx PROPERTIES FOLDER libs/extras)

install(
  TARGETS hilite_cxx
  COMPONENT hilite_cxx
)

install(
  FILES src/hilite/cxx.hh
  DESTINATION include/hilite
  COMPONENT hilite_cxx
)

cpack_add_component(hilite_cxx
  DISPLAY_NAME "libhilite_cxx"
  GROUP extras
)
