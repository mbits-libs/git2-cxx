set(SOURCES
  src/components.cc
  src/link_service.cc
  src/mstch_cache.cc

  include/web/components.hh
  include/web/link_service.hh
  include/web/mstch_cache.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(web ${SOURCES})
target_link_libraries(web PUBLIC ext_platform mstch c++filt)

set_target_properties(web PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  file(GLOB WEB_TEST_SRCS_CC tests/*.cc)
  file(GLOB WEB_TEST_SRCS_CPP tests/*.cpp)
  file(GLOB WEB_TEST_SRCS_CXX tests/*.cxx)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${WEB_TEST_SRCS_CC}
    ${WEB_TEST_SRCS_CPP}
    ${WEB_TEST_SRCS_CXX})

  add_cov_test(web ${WEB_TEST_SRCS_CC} ${WEB_TEST_SRCS_CPP} ${WEB_TEST_SRCS_CXX})
  target_link_libraries(web-test PRIVATE web testing-lib)

  if (COV_CUTDOWN_OS)
    message(STATUS "web tests: removing some tests on cutdown OS")
    target_compile_options(web-test PRIVATE -DCUTDOWN_OS=1)
  endif()

  add_test(NAME web COMMAND web-test "--gtest_output=xml:${TEST_REPORT_DIR}/libexludes/${TEST_REPORT_FILE}")

  if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
    add_dependencies(cov_coveralls_test
      web-test
    )
  endif()
endif()

set_parent_scope()
