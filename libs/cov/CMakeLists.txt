configure_file(src/version.in.hh ${CMAKE_CURRENT_BINARY_DIR}/include/cov/version.hh @ONLY)

set(SOURCES
  src/format/defaults.cc
  src/format/parser.cc
  src/format/formatter.cc
  src/hash/md5.cc
  src/hash/sha1.cc
  src/io/db_object-error.cc
  src/io/db_object.cc
  src/io/file.cc
  src/io/line_coverage.cc
  src/io/read_stream.cc
  src/io/report_files.cc
  src/io/report.cc
  src/io/safe_stream.cc
  src/io/strings.cc
  src/ref/internal.hh
  src/ref/reference_list.cc
  src/ref/reference.cc
  src/ref/references.cc
  src/branch-tag.hh
  src/branch.cc
  src/counted.cc
  src/db.cc
  src/discover.cc
  src/init.cc
  src/module_config.cc
  src/module.cc
  src/path-utils.hh
  src/repository.cc
  src/revparse.cc
  src/streams.cc
  src/tag.cc
  src/zstream.cc
  include/cov/hash/hash.hh
  include/cov/hash/md5.hh
  include/cov/hash/sha1.hh
  include/cov/io/db_object.hh
  include/cov/io/file.hh
  include/cov/io/line_coverage.hh
  include/cov/io/read_stream.hh
  include/cov/io/report_files.hh
  include/cov/io/report.hh
  include/cov/io/safe_stream.hh
  include/cov/io/strings.hh
  include/cov/io/types.hh
  include/cov/branch.hh
  include/cov/counted.hh
  include/cov/db.hh
  include/cov/discover.hh
  include/cov/format_args.hh
  include/cov/format.hh
  include/cov/init.hh
  include/cov/module.hh
  include/cov/object.hh
  include/cov/reference.hh
  include/cov/report.hh
  include/cov/repository.hh
  include/cov/revparse.hh
  include/cov/streams.hh
  include/cov/tag.hh
  include/cov/zstream.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(cov-api STATIC ${SOURCES})
target_compile_options(cov-api PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(cov-api PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(cov-api PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(cov-api PUBLIC git2-c++ fmt::fmt date::date mbits::args PRIVATE hilite openssl::openssl)

set_target_properties(cov-api PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  file(GLOB TEST_SRCS_CC tests/*.cc tests/format/*.cc)
  file(GLOB TEST_SRCS_CPP tests/*.cpp)
  file(GLOB TEST_SRCS_CXX tests/*.cxx)
  file(GLOB STRESS_TEST_SRCS_CC tests/stress/*.cc)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${TEST_SRCS_CC}
    ${TEST_SRCS_CPP}
    ${TEST_SRCS_CXX}
    ${STRESS_TEST_SRCS_CC})

  add_executable(cov-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_executable(cov-stress-test ${STRESS_TEST_SRCS_CC} tests/setup.cc ../git2-c++/tests/stress/new.cc)

  set_target_properties(cov-test cov-stress-test PROPERTIES FOLDER tests)

  target_compile_options(cov-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(cov-test PRIVATE cov-api testing-lib mbits::args)
  target_include_directories(cov-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(cov-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(cov-stress-test PRIVATE cov-api testing-lib)
  target_include_directories(cov-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_SOURCE_DIR}/../git2-c++/tests/stress
      ${CMAKE_CURRENT_BINARY_DIR})

  if (COV_CUTDOWN_OS)
    message(STATUS "cov-test: removing some tests on cutdown OS")
    target_compile_options(cov-test PRIVATE -DCUTDOWN_OS=1)
    target_compile_options(cov-stress-test PRIVATE -DCUTDOWN_OS=1)
  endif()

  add_test(NAME cov COMMAND cov-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcov/${TEST_REPORT_FILE}")
  add_test(NAME cov-stress COMMAND cov-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcov-stress/${TEST_REPORT_FILE}")
endif()

install(
  TARGETS cov-api
  COMPONENT libcov
)

install(
  DIRECTORY include/ "${CMAKE_CURRENT_BINARY_DIR}/include/"
  TYPE INCLUDE
  COMPONENT libcov
)

cpack_add_component(libcov
  DISPLAY_NAME "libcov-api"
  DEPENDS git2_cxx lighter
  GROUP devel
)
