set(LNGS_INSTALL OFF CACHE BOOL "Create install targets")
set(LNGS_TESTING OFF CACHE BOOL "Compile and/or run self-tests")
set(LNGS_APP OFF CACHE BOOL "Compile lngs application in addition to liblngs")
set(LNGS_LITE OFF CACHE BOOL "Create lngs_lite library")

set(JSON_TESTING OFF CACHE BOOL "Compile and/or run self-tests")
set(JSON_INSTALL OFF CACHE BOOL "Install the library")

add_subdirectory(lngs)
add_subdirectory(json)

set_target_properties(json PROPERTIES FOLDER libs)
