cmake_minimum_required (VERSION 3.10)
project (git2-c++
  DESCRIPTION "C++ wrappers for libgit2"
  VERSION 0.1.0
  LANGUAGES CXX
)

set(PROJECT_VERSION_STABILITY "-alpha" # or "-beta", or "-rc.1", or "-rc.2", or ""
    CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}/conan")
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/conan")

set(CONAN_CMAKE_SILENT_OUTPUT ON)
find_package(libgit2 REQUIRED)

include(${CMAKE_BINARY_DIR}/conan/conanbuildinfo.cmake)
conan_basic_setup()

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
	include/git2++/bytes.hh
	include/git2++/blob.hh
	include/git2++/commit.hh
	include/git2++/config.hh
	include/git2++/global.hh
	include/git2++/object.hh
	include/git2++/odb.hh
	include/git2++/ptr.hh
	include/git2++/repository.hh
	include/git2++/submodule.hh
	include/git2++/tree.hh
  )

add_library(git2++ STATIC ${SOURCES})
target_compile_options(git2++ PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(git2++ PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(git2++ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(git2++ PUBLIC libgit2::libgit2)

set_target_properties(git2++ PROPERTIES FOLDER libs)
