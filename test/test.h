#ifndef TEST_H
#define TEST_H

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>

// Test framework similar to Go's testing package
namespace test {

struct TestResult {
    std::string name;
    bool passed;
    std::string error;
    double duration_ms;
};

class TestContext {
private:
    std::string current_test;
    bool failed;
    std::string failure_msg;
    std::vector<TestResult> results;
    size_t passed_count;
    size_t failed_count;
    
public:
    TestContext() : failed(false), passed_count(0), failed_count(0) {}
    
    void StartTest(const std::string& test_name) {
        current_test = test_name;
        failed = false;
        failure_msg = "";
    }
    
    void Fail(const std::string& msg) {
        if (!failed) {
            failed = true;
            failure_msg = msg;
        }
    }
    
    void Failf(const std::string& format, const std::string& value) {
        std::string msg = format;
        size_t pos = msg.find("%s");
        if (pos != std::string::npos) {
            msg.replace(pos, 2, value);
        }
        Fail(msg);
    }
    
    void EndTest(double duration_ms) {
        TestResult result;
        result.name = current_test;
        result.passed = !failed;
        result.error = failure_msg;
        result.duration_ms = duration_ms;
        results.push_back(result);
        
        if (result.passed) {
            passed_count++;
        } else {
            failed_count++;
        }
        
        current_test = "";
        failed = false;
        failure_msg = "";
    }
    
    void PrintResults() {
        std::cout << "\n=== Test Results ===" << std::endl;
        for (const auto& result : results) {
            std::string status = result.passed ? "PASS" : "FAIL";
            std::string color = result.passed ? "\033[32m" : "\033[31m";
            std::string reset = "\033[0m";
            
            std::cout << color << status << reset << ": " << result.name 
                      << " (" << std::fixed << std::setprecision(2) 
                      << result.duration_ms << "ms)" << std::endl;
            
            if (!result.passed && !result.error.empty()) {
                std::cout << "    " << result.error << std::endl;
            }
        }
        
        std::cout << "\nTotal: " << (passed_count + failed_count) << " tests" << std::endl;
        std::cout << "Passed: " << passed_count << std::endl;
        std::cout << "Failed: " << failed_count << std::endl;
        
        if (failed_count > 0) {
            std::cout << "\n\033[31mFAIL\033[0m" << std::endl;
        } else {
            std::cout << "\n\033[32mPASS\033[0m" << std::endl;
        }
    }
    
    bool HasFailures() const {
        return failed_count > 0;
    }
    
    size_t GetFailedCount() const {
        return failed_count;
    }
};

// Global test context
extern TestContext* g_test_ctx;

// Helper macros
#define TEST(name) \
    void Test##name(test::TestContext& ctx); \
    static void __test_##name##_wrapper(test::TestContext& ctx) { \
        ctx.StartTest(#name); \
        auto start = std::chrono::high_resolution_clock::now(); \
        try { \
            Test##name(ctx); \
        } catch (const std::exception& e) { \
            ctx.Failf("Uncaught exception: %s", e.what()); \
        } catch (...) { \
            ctx.Fail("Uncaught unknown exception"); \
        } \
        auto end = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); \
        ctx.EndTest(duration.count() / 1000.0); \
    } \
    void Test##name(test::TestContext& ctx)

// Assertions
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            ctx.Fail("Assertion failed: " #condition); \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            ctx.Fail("Assertion failed: !" #condition); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::ostringstream oss; \
            oss << "Expected " << (expected) << " but got " << (actual); \
            ctx.Fail(oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_NEQ(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::ostringstream oss; \
            oss << "Expected not equal to " << (expected) << " but got " << (actual); \
            ctx.Fail(oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_NEAR(expected, actual, epsilon) \
    do { \
        double diff = std::abs((expected) - (actual)); \
        if (diff > (epsilon)) { \
            std::ostringstream oss; \
            oss << "Expected " << (expected) << " but got " << (actual) \
                << " (diff: " << diff << ", epsilon: " << (epsilon) << ")"; \
            ctx.Fail(oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        std::string exp_str = (expected); \
        std::string act_str = (actual); \
        if (exp_str != act_str) { \
            std::ostringstream oss; \
            oss << "Expected \"" << exp_str << "\" but got \"" << act_str << "\""; \
            ctx.Fail(oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            ctx.Fail("Expected non-null pointer but got null"); \
            return; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            ctx.Fail("Expected null pointer but got non-null"); \
            return; \
        } \
    } while (0)

} // namespace test

#include <chrono>
#include <functional>
#include <vector>

// Test registry (declared in test_main.cpp)
namespace test {
    extern std::vector<std::pair<std::string, std::function<void(TestContext&)>>> test_functions;
    void RegisterTest(const std::string& name, std::function<void(TestContext&)> func);
}

// Macro to register a test function (now included in DEFINE_TEST)
#define REGISTER_TEST(name) /* Deprecated - use DEFINE_TEST */

// Helper to define test with registration
// Ensure static initialization happens by using a function-scope static
#define DEFINE_TEST(name) \
    static void __test_##name##_impl(test::TestContext& ctx); \
    static void __test_##name##_wrapper(test::TestContext& ctx) { \
        __test_##name##_impl(ctx); \
    } \
    namespace { \
        struct TestRegistrar_##name { \
            TestRegistrar_##name() { \
                test::RegisterTest(#name, __test_##name##_wrapper); \
            } \
        }; \
        static TestRegistrar_##name __test_registrar_##name __attribute__((used)); \
    } \
    static void __test_##name##_impl(test::TestContext& ctx)

#endif // TEST_H