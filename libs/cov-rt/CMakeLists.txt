set(SOURCES
  src/branches.cc
  src/checkout.cc
  src/config.cc
  src/module.cc
  src/path_env.hh
  src/report_command.cc
  src/report.cc
  src/root_command.cc
  src/rt_path.cc
  src/show_range.cc
  src/show.cc
  src/split_command.cc
  src/tools.cc

  include/cov/app/branches.hh
  include/cov/app/checkout.hh
  include/cov/app/config.hh
  include/cov/app/cov_config_tr.hh
  include/cov/app/cov_init_tr.hh
  include/cov/app/cov_module_tr.hh
  include/cov/app/cov_report_tr.hh
  include/cov/app/cov_tr.hh
  include/cov/app/errors_tr.hh
  include/cov/app/module.hh
  include/cov/app/report_command.hh
  include/cov/app/report.hh
  include/cov/app/root_command.hh
  include/cov/app/rt_path.hh
  include/cov/app/show_range.hh
  include/cov/app/show.hh
  include/cov/app/split_command.hh
  include/cov/app/tools.hh
)

if (UNIX)
	list(APPEND SOURCES src/posix.cc)
elseif(WIN32)
	list(APPEND SOURCES src/win32.cc)
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(cov-rt ${SOURCES})
target_link_libraries(cov-rt PUBLIC cov-core)

set_target_properties(cov-rt PROPERTIES FOLDER libs)

if (cov_COVERALLS)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cov-rt PRIVATE -DRUNNING_GCOV=1)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cov-rt PRIVATE -DRUNNING_LLVM_COV=1)
  endif()
endif()

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
    ${STRESS_TEST_SRCS_CC}
  )

  add_cov_test(cov-rt ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_cov_test(cov-rt-stress ${STRESS_TEST_SRCS_CC} tests/setup.cc ../cov-api/tests/git2-c++/stress/new.cc)
  target_link_libraries(cov-rt-test PRIVATE cov-rt testing-lib)
  target_link_libraries(cov-rt-stress-test PRIVATE cov-rt testing-lib)
  target_include_directories(cov-rt-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../cov-api/tests/git2-c++/stress)

  add_test(NAME cov-rt COMMAND cov-rt-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcov-rt/${TEST_REPORT_FILE}")
  add_test(NAME cov-rt-stress COMMAND cov-rt-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/libcov-rt-stress/${TEST_REPORT_FILE}")

  if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
    add_dependencies(cov_coveralls_test
      cov-rt-test cov-rt-stress-test
    )
  endif()
endif()

install(
  TARGETS cov-rt
  COMPONENT librt
)

install(
  DIRECTORY include/
  TYPE INCLUDE
  COMPONENT librt
)

cpack_add_component(librt
  DISPLAY_NAME "libcov-rt"
  GROUP devel
)

set_parent_scope()
