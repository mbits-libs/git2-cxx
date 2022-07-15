set(SOURCES
    src/tr.cc

    include/cov/app/tr.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(app STATIC ${SOURCES})
target_compile_options(app PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(app PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(app PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(app PUBLIC cov-api lighter mbits::args)

set_target_properties(app PROPERTIES FOLDER libs)

install(
  TARGETS app
  COMPONENT libapp
)

install(
  DIRECTORY include/
  TYPE INCLUDE
  COMPONENT libapp
)

cpack_add_component(libapp
  DISPLAY_NAME "libapp"
  GROUP devel
)
