#include "test.h"
#include <chrono>

using namespace std::chrono;

// Test registry implementation
namespace test {
    std::vector<std::pair<std::string, std::function<void(TestContext&)>>> test_functions;
    
    void RegisterTest(const std::string& name, std::function<void(TestContext&)> func) {
        test_functions.push_back({name, func});
    }
}

// Forward declaration
extern void RegisterAllTests();

int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    std::cout << "Running tests..." << std::endl;
    std::cout << "=================" << std::endl;
    
    // Register all tests explicitly
    RegisterAllTests();
    
    std::cout << "Registered " << test::test_functions.size() << " test(s)" << std::endl;
    
    // Run tests
    test::TestContext ctx;
    
    for (const auto& [name, func] : test::test_functions) {
        ctx.StartTest(name);
        auto start = high_resolution_clock::now();
        
        try {
            func(ctx);
        } catch (const std::exception& e) {
            ctx.Failf("Uncaught exception: %s", e.what());
        } catch (...) {
            ctx.Fail("Uncaught unknown exception");
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        ctx.EndTest(duration.count() / 1000.0);
    }
    
    // Print results
    ctx.PrintResults();
    
    // Return exit code (avoid crash on Windows)
    int exit_code = ctx.HasFailures() ? 1 : 0;
    std::cout << "\nExiting with code: " << exit_code << std::endl;
    return exit_code;
}
