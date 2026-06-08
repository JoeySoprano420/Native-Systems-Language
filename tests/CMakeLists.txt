cmake_minimum_required(VERSION 3.16)

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(nsl_tests
    main.cpp
    test_lexer.cpp
    test_parser.cpp
    test_sema.cpp
    test_codegen.cpp
)
target_link_libraries(nsl_tests PRIVATE nsl_lib)
target_include_directories(nsl_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Each test function is run as a CTest test via the test runner
add_test(NAME nsl_tests COMMAND nsl_tests)
