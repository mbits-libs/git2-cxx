cmake_minimum_required (VERSION 3.10)
project (git2-c++
  DESCRIPTION "C++ wrappers for libgit2"
  VERSION 0.1.0
  LANGUAGES CXX
)

set(PROJECT_VERSION_STABILITY "-alpha" # or "-beta", or "-rc.1", or "-rc.2", or ""
    CACHE STRING "" FORCE)

set(GIT2_CXX_TESTING ON CACHE BOOL "Compile and/or run self-tests")

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

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_options(${PROJECT_NAME} PRIVATE ${ADDITIONAL_LINK_FLAGS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC libgit2::libgit2)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER libs)

if (GIT2_CXX_TESTING)
  find_package(GTest REQUIRED CONFIG HINTS)

  set(COVERALLS_PREFIX git2_cxx_)
  set(git2_cxx_COVERALLS_DIRS include src)
  include(tools/coveralls/Coveralls.cmake)

  set(EXTERNAL_GTEST ON)
  include(tools/testing/googletest.cmake)

  add_test_executable(git2-c++-test DATA_PATH repos LIBRARIES git2-c++)
  add_test(NAME git2-c++ COMMAND git2-c++-test)
endif()
