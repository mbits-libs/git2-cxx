set(SOURCES
  src/blob.cc
  src/commit.cc
  src/config.cc
  src/error.cc
  src/global.cc
  src/object.cc
  src/odb.cc
  src/ptr.cc
  src/repository.cc
  src/submodule.cc
  src/transaction.cc
  src/tree.cc
  include/git2/bytes.hh
  include/git2/blob.hh
  include/git2/commit.hh
  include/git2/config.hh
  include/git2/diff.hh
  include/git2/error.hh
  include/git2/global.hh
  include/git2/object.hh
  include/git2/oid.hh
  include/git2/odb.hh
  include/git2/ptr.hh
  include/git2/repository.hh
  include/git2/submodule.hh
  include/git2/transaction.hh
  include/git2/tree.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(git2-c++ STATIC ${SOURCES})
target_compile_options(git2-c++ PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(git2-c++ PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(git2-c++ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(git2-c++ PUBLIC libgit2::libgit2)

set_target_properties(git2-c++ PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  file(GLOB TEST_SRCS_CC tests/*.cc)
  file(GLOB TEST_SRCS_CPP tests/*.cpp)
  file(GLOB TEST_SRCS_CXX tests/*.cxx)
  file(GLOB STRESS_TEST_SRCS_CC tests/stress/*.cc)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests FILES
    ${TEST_SRCS_CC}
    ${TEST_SRCS_CPP}
    ${TEST_SRCS_CXX}
    ${STRESS_TEST_SRCS_CC})

  add_executable(git2-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_executable(git2-stress-test ${STRESS_TEST_SRCS_CC} tests/setup.cc)

  set_target_properties(git2-test git2-stress-test PROPERTIES FOLDER tests)

  target_compile_options(git2-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(git2-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(git2-test PRIVATE git2-c++ testing-lib)
  target_include_directories(git2-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(git2-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(git2-stress-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(git2-stress-test PRIVATE git2-c++ testing-lib)
  target_include_directories(git2-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  if (COV_CUTDOWN_OS)
    message(STATUS "git2-test: removing some tests on cutdown OS")
    target_compile_options(git2-test PRIVATE -DCUTDOWN_OS=1)
    target_compile_options(git2-stress-test PRIVATE -DCUTDOWN_OS=1)
  endif()

  add_test(NAME git2-c++ COMMAND git2-test "--gtest_output=xml:${TEST_REPORT_DIR}/libgit2-c++/${TEST_REPORT_FILE}")
  add_test(NAME git2-stress COMMAND git2-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/libgit2-c++-stress/${TEST_REPORT_FILE}")
endif()

install(
  TARGETS git2-c++
  COMPONENT git2_cxx
)

install(
  DIRECTORY include/
  TYPE INCLUDE
  COMPONENT git2_cxx
)

cpack_add_component(git2_cxx
  DISPLAY_NAME "libgit2-c++"
  GROUP devel
)
