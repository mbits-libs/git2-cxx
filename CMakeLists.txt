cmake_minimum_required (VERSION 3.20)
project (cov
  DESCRIPTION "Code coverage presented localy"
  VERSION 0.12.0
  LANGUAGES CXX
)

set(PROJECT_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PROJECT_VERSION_STABILITY "-beta" # or "-alpha", or "-rc.1", or "-rc.2", or ""
    CACHE STRING "" FORCE)

set(STRINGS args cov errors cov_init cov_config)
set(LANGUAGES pl en)

set(COV_TESTING ON CACHE BOOL "Compile and/or run self-tests")
set(COV_CUTDOWN_OS OFF CACHE BOOL "Run tests on cutdown OS (e.g. GitHub docker)")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}/conan" "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/conan")

set(CONAN_CMAKE_SILENT_OUTPUT ON)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(libgit2 REQUIRED)
find_package(fmt REQUIRED)
find_package(date REQUIRED)
find_package(args REQUIRED)
find_program(LNGS lngs)

if (LNGS)
  message(STATUS "lngs is ${LNGS}")
endif()

if (COV_TESTING)
  enable_testing()

  find_package(GTest REQUIRED)

  set(COVERALLS_PREFIX cov_)
  set(cov_COVERALLS_DIRS
    apps/builtins
    apps/cov
  )

  foreach(module
    libs/git2-c++
    libs/cov
    libs/cell
    libs/hilite/hilite
    libs/hilite/lighter
    libs/app
    libs/cov-rt
  )
    list(APPEND cov_COVERALLS_DIRS ${module}/include ${module}/src)
  endforeach()
  foreach(extra cxx py3 ts)
    list(APPEND cov_COVERALLS_DIRS libs/hilite/hilite-${extra}/src)
  endforeach()
  include(${PROJECT_SOURCE_DIR}/tools/coveralls/Coveralls.cmake)

  set(TEST_REPORT_DIR "${PROJECT_BINARY_DIR}/test-results")
  set(TEST_REPORT_FILE "${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}.xml")
endif()

include(${CMAKE_BINARY_DIR}/conan/conanbuildinfo.cmake)
conan_basic_setup()

if (MSVC)
  set(ADDITIONAL_WALL_FLAGS
      /D_CRT_SECURE_NO_WARNINGS
      /D_CRT_NONSTDC_NO_WARNINGS
      /utf-8
      /permissive-
      /Zc:__cplusplus
      /W4
      /w14242
      /w14254
      /w14263
      /w14265
      /w14287
      /we4289
      /w14296
      /w14311
      /w14545
      /w14546
      /w14547
      /w14549
      /w14555
      /w14619
      /w14640
      /w14826
      /w14905
      /w14906
      /w14928
      /w14946)
else()
  set(ADDITIONAL_WALL_FLAGS
      -Wall -Wextra
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Wcast-align
      -Wunused
      -Woverloaded-virtual
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
  )
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND ADDITIONAL_WALL_FLAGS -fcolor-diagnostics) # -Wlifetime
  else()
    list(APPEND ADDITIONAL_WALL_FLAGS
      -fdiagnostics-color
      -Wmisleading-indentation
      -Wduplicated-cond
      -Wduplicated-branches
      -Wlogical-op
      -Wuseless-cast
      )
  endif()
endif()

include(prepare_pack)

set(BINARY_DIR bin)
set(ROOT_DIR "..")
set(SHARE_DIR share/cov-${PROJECT_VERSION_SHORT})
set(CORE_DIR libexec/cov)
set(CORE_ROOT_DIR "../..")

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(apps)


if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET cov_coveralls_test)
  add_dependencies(cov_coveralls_test
    git2-test git2-stress-test
    cov-test cov-stress-test
    lighter-test lighter-stress-test
    cov
  )
endif()
