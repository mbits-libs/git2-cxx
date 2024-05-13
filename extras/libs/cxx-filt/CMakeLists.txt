set(SOURCES
  src/expression.cc
  src/parser.cc
  src/types.cc

  include/cxx-filt/expression.hh
  include/cxx-filt/parser.hh
  include/cxx-filt/types.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(cxx-filt ${SOURCES})
target_link_libraries(cxx-filt PRIVATE fmt::fmt)

set_target_properties(cxx-filt PROPERTIES FOLDER libs)

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

  add_cov_test(cxx-filt ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  target_link_libraries(cxx-filt-test PRIVATE cxx-filt fmt::fmt testing-lib)

  add_test(NAME cxx-filt COMMAND cxx-filt-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcxx-filt/${TEST_REPORT_FILE}")

  if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
    add_dependencies(cov_coveralls_test
      cxx-filt-test
    )
  endif()
endif()

set_parent_scope()
