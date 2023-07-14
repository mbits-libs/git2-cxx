set(SOURCES
  src/lighter.cc
  src/hilite/lighter.hh
  )
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

set(DEPS)
foreach(hilite cxx py3 ts)
  if (TARGET hilite-${hilite})
    list(APPEND DEPS hilite-${hilite})
  endif()
endforeach()
if (NOT DEPS)
  list(APPEND DEPS hilite)
else()
string(REPLACE ";" ", " DEP_LIST "${DEPS}")
  message(STATUS "Syntax: ${DEP_LIST}")
endif()


add_library(lighter STATIC ${SOURCES})
target_compile_options(lighter PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(lighter PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(lighter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(lighter PUBLIC ${DEPS})
set_target_properties(lighter PROPERTIES FOLDER libs/hilite)

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

  add_executable(lighter-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
  add_executable(lighter-stress-test ${STRESS_TEST_SRCS_CC}
    ../../cov-api/tests/git2-c++/stress/new.cc)

  set_target_properties(lighter-test lighter-stress-test PROPERTIES FOLDER tests)

  target_compile_options(lighter-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(lighter-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(lighter-test lighter GTest::gmock_main fmt::fmt)
  target_include_directories(lighter-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_options(lighter-stress-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
  target_link_options(lighter-stress-test PRIVATE ${ADDITIONAL_LINK_FLAGS})
  target_link_libraries(lighter-stress-test PRIVATE lighter GTest::gmock_main)
  target_include_directories(lighter-stress-test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_SOURCE_DIR}/../../cov-api/tests/git2-c++/stress
      ${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME lighter COMMAND lighter-test "--gtest_output=xml:${TEST_REPORT_DIR}/liblighter/${TEST_REPORT_FILE}")
  add_test(NAME lighter-stress COMMAND lighter-stress-test "--gtest_output=xml:${TEST_REPORT_DIR}/liblighter-stress/${TEST_REPORT_FILE}")
endif()

install(
  TARGETS lighter
  COMPONENT lighter
)

install(
  FILES src/hilite/lighter.hh
  DESTINATION include/hilite
  COMPONENT lighter
)

set(_DEPS)

foreach (DEP ${DEPS})
  string(REPLACE "-" "_" _DEP ${DEP})
  list(APPEND _DEPS ${_DEP})
endforeach()

cpack_add_component(lighter
  DISPLAY_NAME "liblighter"
  DEPENDS ${_DEPS}
  GROUP devel
)
