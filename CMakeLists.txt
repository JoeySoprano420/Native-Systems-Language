cmake_minimum_required(VERSION 3.16)

project(NSL
    VERSION 1.0.0
    DESCRIPTION "Native Systems Language Compiler (nslc)"
    LANGUAGES CXX
)

# ── C++ standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type default ───────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── Compiler warnings ────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
elseif(MSVC)
    add_compile_options(/W4 /wd4100)
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
set(NSL_SOURCES
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/types.cpp
    src/diagnostics.cpp
    src/sema.cpp
    src/ir.cpp
    src/irgen.cpp
    src/codegen.cpp
    src/compiler.cpp
)

# ── Compiler library (reusable by tests) ─────────────────────────────────────
add_library(nsl_lib STATIC ${NSL_SOURCES})
target_include_directories(nsl_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── nslc executable ───────────────────────────────────────────────────────────
add_executable(nslc src/main.cpp)
target_link_libraries(nslc PRIVATE nsl_lib)
target_include_directories(nslc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(NSL_BUILD_TESTS "Build NSL compiler tests" ON)

if(NSL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install rules ────────────────────────────────────────────────────────────
install(TARGETS nslc RUNTIME DESTINATION bin)
install(DIRECTORY examples/ DESTINATION share/nsl/examples
        FILES_MATCHING PATTERN "*.nsl")

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "NSL Compiler Configuration")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Tests:      ${NSL_BUILD_TESTS}")
message(STATUS "")
