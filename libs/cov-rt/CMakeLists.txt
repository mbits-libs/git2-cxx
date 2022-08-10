set(SOURCES
  src/config.cc
  src/module.cc
  src/report_command.cc
  src/report.cc
  src/root_command.cc
  src/rt_path.cc
  src/split_command.cc
  src/tools.cc

  include/cov/app/config.hh
  include/cov/app/module.hh
  include/cov/app/cov_config_tr.hh
  include/cov/app/cov_init_tr.hh
  include/cov/app/cov_module_tr.hh
  include/cov/app/cov_report_tr.hh
  include/cov/app/cov_tr.hh
  include/cov/app/errors_tr.hh
  include/cov/app/report_command.hh
  include/cov/app/report.hh
  include/cov/app/root_command.hh
  include/cov/app/rt_path.hh
  include/cov/app/split_command.hh
  include/cov/app/tools.hh
)

if (UNIX)
	list(APPEND SOURCES src/posix.cc)
elseif(WIN32)
	list(APPEND SOURCES src/win32.cc)
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_library(cov-rt STATIC ${SOURCES})
target_compile_options(cov-rt PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(cov-rt PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(cov-rt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(cov-rt PUBLIC app json)

set_target_properties(cov-rt PROPERTIES FOLDER libs)

if (cov_COVERALLS AND NOT MSVC)
  target_compile_options(cov-rt PRIVATE -DRUNNING_GCOV=1)
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

  add_executable(cov-rt-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_executable(cov-rt-stress-test ${STRESS_TEST_SRCS_CC} tests/setup.cc ../git2-c++/tests/stress/new.cc)

  set_target_properties(cov-rt-test cov-rt-stress-test PROPERTIES FOLDER tests)

  target_compile_options(cov-rt-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(cov-rt-test PRIVATE cov-rt testing-lib)
  target_include_directories(cov-rt-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(cov-rt-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_libraries(cov-rt-stress-test PRIVATE cov-rt testing-lib)
  target_include_directories(cov-rt-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_SOURCE_DIR}/../git2-c++/tests/stress
      ${CMAKE_CURRENT_BINARY_DIR})

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
