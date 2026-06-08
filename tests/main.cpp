// NSL Test Runner – main entry point for all tests

#include <iostream>
#include <cstdlib>

// Test suite declarations
void run_lexer_tests();
void run_parser_tests();
void run_sema_tests();
void run_codegen_tests();

int g_pass = 0;
int g_fail = 0;

int main() {
    std::cout << "NSL Compiler Test Suite\n";
    std::cout << "=======================\n\n";

    run_lexer_tests();
    run_parser_tests();
    run_sema_tests();
    run_codegen_tests();

    std::cout << "\n=======================\n";
    std::cout << "Results: " << g_pass << " passed, " << g_fail << " failed\n";

    if (g_fail > 0) {
        std::cout << "SOME TESTS FAILED\n";
        return 1;
    }
    std::cout << "ALL TESTS PASSED\n";
    return 0;
}
