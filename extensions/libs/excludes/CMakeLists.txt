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

if (COV_TESTING)
  enable_testing()

  file(GLOB EXCLUDES_TEST_SRCS_CC tests/*.cc)
  file(GLOB EXCLUDES_TEST_SRCS_CPP tests/*.cpp)
  file(GLOB EXCLUDES_TEST_SRCS_CXX tests/*.cxx)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${EXCLUDES_TEST_SRCS_CC}
    ${EXCLUDES_TEST_SRCS_CPP}
    ${EXCLUDES_TEST_SRCS_CXX})

  add_executable(strip-excludes-test ${EXCLUDES_TEST_SRCS_CC} ${EXCLUDES_TEST_SRCS_CPP} ${EXCLUDES_TEST_SRCS_CXX})
  set_target_properties(strip-excludes-test PROPERTIES FOLDER tests)

  target_compile_options(strip-excludes-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(strip-excludes-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(strip-excludes-test PRIVATE excludes testing-lib)
  target_include_directories(strip-excludes-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  if (COV_CUTDOWN_OS)
    message(STATUS "strip-excludes tests: removing some tests on cutdown OS")
    target_compile_options(strip-excludes-test PRIVATE -DCUTDOWN_OS=1)
  endif()

  add_test(NAME strip-excludes COMMAND strip-excludes-test "--gtest_output=xml:${TEST_REPORT_DIR}/libexludes/${TEST_REPORT_FILE}")
endif()
