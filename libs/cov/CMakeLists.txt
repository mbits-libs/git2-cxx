configure_file(src/version.in.hh ${CMAKE_CURRENT_BINARY_DIR}/include/cov/version.hh @ONLY)

set(SOURCES
	src/hash/md5.cc
	src/hash/sha1.cc
	src/io/file.cc
	src/io/read_stream.cc
	src/io/safe_stream.cc
	src/init.cc
	src/path-utils.hh
	src/streams.cc
	src/zstream.cc
	include/cov/hash/hash.hh
	include/cov/hash/md5.hh
	include/cov/hash/sha1.hh
	include/cov/io/file.hh
	include/cov/io/read_stream.hh
	include/cov/io/safe_stream.hh
	include/cov/init.hh
	include/cov/streams.hh
	include/cov/zstream.hh
)

add_library(libcov STATIC ${SOURCES})
target_compile_options(libcov PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(libcov PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(libcov PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(libcov PUBLIC git2-c++ OpenSSL::OpenSSL)

set_target_properties(libcov PROPERTIES FOLDER libs OUTPUT_NAME cov)

if (COV_TESTING)
  enable_testing()

  find_package(GTest REQUIRED CONFIG HINTS)

	file(GLOB TEST_SRCS_CC tests/*.cc)
	file(GLOB TEST_SRCS_CPP tests/*.cpp)
	file(GLOB TEST_SRCS_CXX tests/*.cxx)
	add_executable(cov-test ${TEST_SRCS_CC} ${TEST_SRCS_CPP} ${TEST_SRCS_CXX})
	set_target_properties(cov-test PROPERTIES FOLDER tests)
	target_link_libraries(cov-test libcov GTest::gtest_main)
	target_include_directories(cov-test
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/${TEST_EXE_TEST_SUBDIR}
			${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME cov COMMAND cov-test)
endif()
