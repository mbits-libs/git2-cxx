macro(mock_ex name source linkname)
    add_executable(mock-${name} mock_${source}.cc)
    set_target_properties(mock-${name} PROPERTIES OUTPUT_NAME ${linkname})
    target_link_libraries(mock-${name} PRIVATE fmt::fmt mbits::args app)
    set_target_properties(mock-${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mocks")
    fix_vs_modules(mock-${name})

    foreach(BUILD_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(
            mock-${name}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/mocks"
        )
    endforeach()
endmacro()

macro(mock name linkname)
    string(REPLACE "-" "_" SAFE ${name})
    mock_ex(${name} ${SAFE} ${linkname})
endmacro()

if (COV_TESTING)
    mock(gcc hal9000-lcars-GLaDOS-g++-17)
    mock(gcov hal9000-lcars-GLaDOS-gcov-17)
    mock(clang clang++-17)
    mock(llvm-cov llvm-cov-17)
    mock(llvm-profdata llvm-profdata-17)
    mock(cl cl)
    mock(occ OpenCppCoverage)

    set_target_properties(mock-cl PROPERTIES SUFFIX ".exe")
endif()
