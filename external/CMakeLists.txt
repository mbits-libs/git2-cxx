set(LNGS_INSTALL OFF CACHE BOOL "Create install targets")
set(LNGS_TESTING OFF CACHE BOOL "Compile and/or run self-tests")
set(LNGS_APP OFF CACHE BOOL "Compile lngs application in addition to liblngs")
set(LNGS_LITE OFF CACHE BOOL "Create lngs_lite library")

add_subdirectory(lngs)
