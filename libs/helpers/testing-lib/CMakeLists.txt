if (COV_TESTING)
    set(SOURCES
        src/path.cc
        src/tar.cc
        include/path.hh
        include/tar.hh
    )
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

    add_cov_library(testing-lib ${SOURCES})
    target_link_libraries(testing-lib PUBLIC GTest::gmock_main fmt::fmt)
    set_target_properties(testing-lib PROPERTIES FOLDER tests/libs)
endif()

set_parent_scope()
