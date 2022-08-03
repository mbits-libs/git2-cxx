if (COV_TESTING)
    set(SOURCES
        src/path.cc
        src/tar.cc
        include/path.hh
        include/tar.hh
    )
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

    add_library(testing-lib STATIC ${SOURCES})
    target_compile_options(testing-lib PRIVATE ${ADDITIONAL_WALL_FLAGS})
    target_link_options(testing-lib PRIVATE ${ADDITIONAL_LINK_FLAGS})
    target_include_directories(testing-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(testing-lib PUBLIC GTest::gmock_main fmt::fmt)
    set_target_properties(testing-lib PROPERTIES FOLDER tests/libs)
endif()
