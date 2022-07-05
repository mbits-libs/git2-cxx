set(SOURCES
  src/highlighter.cc
  src/hilite/highlighter.hh
  )
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

set(DEPS)
foreach(hilite cxx ps3 ts)
  if (TARGET hilite-${hilite})
    list(APPEND DEPS hilite-${hilite})
  endif()
endforeach()
if (NOT DEPS)
  list(APPEND DEPS hilite)
endif()


add_library(highlighter STATIC ${SOURCES})
target_compile_options(highlighter PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(highlighter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(highlighter PUBLIC ${DEPS})
set_target_properties(highlighter PROPERTIES FOLDER libs/extras)

if (COV_TESTING)
  enable_testing()

  find_package(GTest REQUIRED CONFIG HINTS)

  file(GLOB TEST_SRCS_CC tests/*.cc)
  file(GLOB TEST_SRCS_CPP tests/*.cpp)
  file(GLOB TEST_SRCS_CXX tests/*.cxx)
  file(GLOB STRESS_TEST_SRCS_CC tests/stress/*.cc)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${TEST_SRCS_CC}
    ${TEST_SRCS_CPP}
    ${TEST_SRCS_CXX}
    ${STRESS_TEST_SRCS_CC})

  add_executable(highlighter-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_executable(highlighter-stress-test ${STRESS_TEST_SRCS_CC}
    ../../libs/git2-c++/tests/stress/new.cc)

  set_target_properties(highlighter-test highlighter-stress-test PROPERTIES FOLDER tests)

  target_compile_options(highlighter-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(highlighter-test highlighter GTest::gmock_main fmt::fmt)
  target_include_directories(highlighter-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(highlighter-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(highlighter-stress-test PRIVATE highlighter GTest::gtest_main)
  target_include_directories(highlighter-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/git2-c++/tests/stress
      ${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME highlighter COMMAND highlighter-test "--gtest_output=xml:${TEST_REPORT_DIR}/libhighlighter/${TEST_REPORT_FILE}")
  add_test(NAME highlighter-stress COMMAND highlighter-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/libhighlighter-stress/${TEST_REPORT_FILE}")
endif()

install(
  TARGETS highlighter
  COMPONENT highlighter
)

install(
  FILES src/hilite/highlighter.hh
  DESTINATION include/hilite
  COMPONENT highlighter
)

cpack_add_component(highlighter
  DISPLAY_NAME "libhighlighter"
  DEPENDS ${DEPS}
  GROUP extras
)
