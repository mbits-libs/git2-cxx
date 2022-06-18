set(SOURCES
  src/blob.cc
  src/commit.cc
  src/config.cc
  src/global.cc
  src/object.cc
  src/odb.cc
  src/ptr.cc
  src/repository.cc
  src/submodule.cc
  src/tree.cc
  include/git2-c++/bytes.hh
  include/git2-c++/blob.hh
  include/git2-c++/commit.hh
  include/git2-c++/config.hh
  include/git2-c++/global.hh
  include/git2-c++/object.hh
  include/git2-c++/odb.hh
  include/git2-c++/ptr.hh
  include/git2-c++/repository.hh
  include/git2-c++/submodule.hh
  include/git2-c++/tree.hh
)

add_library(git2-c++ STATIC ${SOURCES})
target_compile_options(git2-c++ PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(git2-c++ PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(git2-c++ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(git2-c++ PUBLIC libgit2::libgit2)

set_target_properties(git2-c++ PROPERTIES FOLDER libs)

if (COV_TESTING)
  enable_testing()

  find_package(GTest REQUIRED CONFIG HINTS)

	file(GLOB TEST_SRCS_CC tests/*.cc)
	file(GLOB TEST_SRCS_CPP tests/*.cpp)
	file(GLOB TEST_SRCS_CXX tests/*.cxx)
	add_executable(git2-c++-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
	set_target_properties(git2-c++-test PROPERTIES FOLDER tests)
	target_link_libraries(git2-c++-test git2-c++ GTest::gtest_main)
	target_include_directories(git2-c++-test
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/${TEST_EXE_TEST_SUBDIR}
			${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME git2-c++ COMMAND git2-c++-test)
endif()
