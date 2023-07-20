configure_file(src/cov/version.in.hh ${CMAKE_CURRENT_BINARY_DIR}/include/cov/version.hh @ONLY)

set(GIT2_SOURCES
  src/git2-c++/blob.cc
  src/git2-c++/commit.cc
  src/git2-c++/config.cc
  src/git2-c++/error.cc
  src/git2-c++/global.cc
  src/git2-c++/object.cc
  src/git2-c++/odb.cc
  src/git2-c++/ptr.cc
  src/git2-c++/repository.cc
  src/git2-c++/submodule.cc
  src/git2-c++/transaction.cc
  src/git2-c++/tree.cc
  include/cov/git2/bytes.hh
  include/cov/git2/blob.hh
  include/cov/git2/commit.hh
  include/cov/git2/config.hh
  include/cov/git2/diff.hh
  include/cov/git2/error.hh
  include/cov/git2/global.hh
  include/cov/git2/object.hh
  include/cov/git2/oid.hh
  include/cov/git2/odb.hh
  include/cov/git2/ptr.hh
  include/cov/git2/repository.hh
  include/cov/git2/submodule.hh
  include/cov/git2/transaction.hh
  include/cov/git2/tree.hh
)

set(COV_SOURCES
  src/cov/branch-tag.hh
  src/cov/branch.cc
  src/cov/counted.cc
  src/cov/db.cc
  src/cov/discover.cc
  src/cov/error.cc
  src/cov/format/defaults.cc
  src/cov/format/facades.cc
  src/cov/format/internal_environment.hh
  src/cov/format/formatter.cc
  src/cov/format/parser.cc
  src/cov/format/tabular.cc
  src/cov/hash/md5.cc
  src/cov/hash/sha1.cc
  src/cov/init.cc
  src/cov/io/build.cc
  src/cov/io/db_object-error.cc
  src/cov/io/db_object.cc
  src/cov/io/file.cc
  src/cov/io/files.cc
  src/cov/io/function_coverage.cc
  src/cov/io/line_coverage.cc
  src/cov/io/read_stream.cc
  src/cov/io/report.cc
  src/cov/io/safe_stream.cc
  src/cov/io/strings.cc
  src/cov/module_config.cc
  src/cov/module.cc
  src/cov/path-utils.hh
  src/cov/projection.cc
  src/cov/ref/internal.hh
  src/cov/ref/reference_list.cc
  src/cov/ref/reference.cc
  src/cov/ref/references.cc
  src/cov/repository.cc
  src/cov/revparse.cc
  src/cov/streams.cc
  src/cov/tag.cc
  src/cov/zstream.cc
  include/cov/branch.hh
  include/cov/counted.hh
  include/cov/db.hh
  include/cov/discover.hh
  include/cov/error.hh
  include/cov/format_args.hh
  include/cov/format.hh
  include/cov/hash/hash.hh
  include/cov/hash/md5.hh
  include/cov/hash/sha1.hh
  include/cov/init.hh
  include/cov/io/build.hh
  include/cov/io/db_object.hh
  include/cov/io/file.hh
  include/cov/io/files.hh
  include/cov/io/function_coverage.hh
  include/cov/io/line_coverage.hh
  include/cov/io/read_stream.hh
  include/cov/io/report.hh
  include/cov/io/safe_stream.hh
  include/cov/io/strings.hh
  include/cov/io/types.hh
  include/cov/module.hh
  include/cov/object.hh
  include/cov/projection.hh
  include/cov/reference.hh
  include/cov/report.hh
  include/cov/repository.hh
  include/cov/revparse.hh
  include/cov/streams.hh
  include/cov/tabular.hh
  include/cov/tag.hh
  include/cov/zstream.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GIT2_SOURCES} ${COV_SOURCES})

add_library(cov-api STATIC ${GIT2_SOURCES} ${COV_SOURCES})
target_compile_options(cov-api PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(cov-api PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(cov-api PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(cov-api PUBLIC libgit2::libgit2 fmt::fmt date::date-tz mbits::args json PRIVATE hilite openssl::openssl)

set_target_properties(cov-api PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  file(GLOB GIT2_TEST_SRCS_CC tests/git2-c++/*.cc)
  file(GLOB GIT2_TEST_SRCS_CPP tests/git2-c++/*.cpp)
  file(GLOB GIT2_TEST_SRCS_CXX tests/git2-c++/*.cxx)
  file(GLOB GIT2_STRESS_TEST_SRCS_CC tests/git2-c++/stress/*.cc)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests/git2-c++ FILES
    ${GIT2_TEST_SRCS_CC}
    ${GIT2_TEST_SRCS_CPP}
    ${GIT2_TEST_SRCS_CXX}
    ${GIT2_STRESS_TEST_SRCS_CC})

  file(GLOB COV_TEST_SRCS_CC tests/cov/*.cc tests/cov/format/*.cc)
  file(GLOB COV_TEST_SRCS_CPP tests/cov/*.cpp)
  file(GLOB COV_TEST_SRCS_CXX tests/cov/*.cxx)
  file(GLOB COV_STRESS_TEST_SRCS_CC tests/cov/stress/*.cc)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests/cov FILES
    ${COV_TEST_SRCS_CC}
    ${COV_TEST_SRCS_CPP}
    ${COV_TEST_SRCS_CXX}
    ${COV_STRESS_TEST_SRCS_CC})

  add_executable(git2-test ${GIT2_TEST_SRCS_CC} ${GIT2_TEST_SRCS_CPP} ${GIT2_TEST_SRCS_CXX})
  add_executable(git2-stress-test ${GIT2_STRESS_TEST_SRCS_CC} tests/git2-c++/setup.cc)
  add_executable(cov-test ${COV_TEST_SRCS_CC} ${COV_TEST_SRCS_CPP} ${COV_TEST_SRCS_CXX})
  add_executable(cov-stress-test ${COV_STRESS_TEST_SRCS_CC} tests/cov/setup.cc tests/git2-c++/stress/new.cc)

  set_target_properties(git2-test git2-stress-test cov-test cov-stress-test PROPERTIES FOLDER tests)

  target_compile_options(git2-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(git2-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(git2-test PRIVATE cov-api testing-lib)
  target_include_directories(git2-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(git2-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(git2-stress-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(git2-stress-test PRIVATE cov-api testing-lib)
  target_include_directories(git2-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(cov-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(cov-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(cov-test PRIVATE cov-api testing-lib mbits::args)
  target_include_directories(cov-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(cov-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(cov-stress-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(cov-stress-test PRIVATE cov-api testing-lib)
  target_include_directories(cov-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_SOURCE_DIR}/../cov-api/tests/git2-c++/stress
      ${CMAKE_CURRENT_BINARY_DIR})

  if (COV_CUTDOWN_OS)
    message(STATUS "cov-api tests: removing some tests on cutdown OS")
    target_compile_options(git2-test PRIVATE -DCUTDOWN_OS=1)
    target_compile_options(git2-stress-test PRIVATE -DCUTDOWN_OS=1)
    target_compile_options(cov-test PRIVATE -DCUTDOWN_OS=1)
    target_compile_options(cov-stress-test PRIVATE -DCUTDOWN_OS=1)
  endif()

  add_test(NAME git2-c++ COMMAND git2-test "--gtest_output=xml:${TEST_REPORT_DIR}/libgit2-c++/${TEST_REPORT_FILE}")
  add_test(NAME git2-stress COMMAND git2-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/libgit2-c++-stress/${TEST_REPORT_FILE}")
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
  DEPENDS lighter
  GROUP devel
)
