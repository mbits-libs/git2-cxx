set(SOURCES
  src/expression.cc
  src/json.cc
  src/parser.cc
  src/types.cc

  include/c++filt/expression.hh
  include/c++filt/json.hh
  include/c++filt/parser.hh
  include/c++filt/types.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(c++filt ${SOURCES})
target_link_libraries(c++filt PRIVATE fmt::fmt json)

set_target_properties(c++filt PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  file(GLOB TEST_SRCS_CC tests/*.cc)
  file(GLOB TEST_SRCS_CPP tests/*.cpp)
  file(GLOB TEST_SRCS_CXX tests/*.cxx)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${TEST_SRCS_CC}
    ${TEST_SRCS_CPP}
    ${TEST_SRCS_CXX}
  )

  add_cov_test(c++filt ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  target_link_libraries(c++filt-test PRIVATE c++filt fmt::fmt testing-lib)

  add_test(NAME c++filt COMMAND c++filt-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcxx-filt/${TEST_REPORT_FILE}")

  if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
    add_dependencies(cov_coveralls_test
      c++filt-test
    )
  endif()
endif()

set_parent_scope()
