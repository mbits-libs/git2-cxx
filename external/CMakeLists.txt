set(BUILD_TZ_LIB ON CACHE BOOL "build/install of TZ library")
set(JSON_TESTING OFF CACHE BOOL "Compile and/or run self-tests")
set(JSON_INSTALL OFF CACHE BOOL "Install the library")

add_subdirectory(date)
add_subdirectory(json)

set_target_properties(json PROPERTIES FOLDER libs)
