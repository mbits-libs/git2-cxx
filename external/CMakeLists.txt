set(BUILD_TZ_LIB ON CACHE BOOL "build/install of TZ library")
set(JSON_TESTING OFF CACHE BOOL "Compile and/or run self-tests")
set(JSON_INSTALL OFF CACHE BOOL "Install the library")
set(ENABLE_DATE_INSTALL OFF CACHE BOOL "Enable unit tests")

add_library(arch::deps INTERFACE IMPORTED)
target_link_libraries(arch::deps INTERFACE
  libzip::zip
  ZLIB::ZLIB
  BZip2::BZip2
  LibLZMA::LibLZMA)

add_subdirectory(libarch)
add_subdirectory(date)
add_subdirectory(json)

set_target_properties(json PROPERTIES FOLDER libs)

fix_vs_modules(arch)
fix_vs_modules(date-tz)
fix_vs_modules(json)

set_parent_scope()
