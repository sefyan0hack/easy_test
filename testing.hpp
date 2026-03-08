#pragma once
#include <functional>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <meta>
#include <ranges>
#include <chrono>

namespace testing {

    constexpr char const* COLOR_RED    = "\x1b[31m";
    constexpr char const* COLOR_GREEN  = "\x1b[32m";
    constexpr char const* COLOR_YELLOW = "\x1b[33m";
    constexpr char const* COLOR_BOLD   = "\x1b[1m";
    constexpr char const* COLOR_RESET  = "\x1b[0m";

    constexpr std::string_view SEP = "===================================================";

    inline std::function<void(const char* what, const char* file, int line)> current_yield = nullptr;
    using TestFuncType = std::function<void(void)>;

    // nice c++ type system :)
    inline std::unordered_map<
        std::string_view, // test suite
        std::vector<
            std::pair<
                std::string_view, // test name
                TestFuncType // test function
            >
        >
    > all_tests;

    template<auto = [](){}>
    inline auto collect_tests() -> size_t {
        using namespace std::meta;
        constexpr auto ctx = access_context::unprivileged();

        constexpr auto inner_namespaces_of = [](info ns) {
            return members_of(ns, ctx)
                | std::views::filter(is_namespace)
                | std::views::filter(has_identifier);
        };

        constexpr auto get_namespace_by_id = [inner_namespaces_of](std::string_view name, info in = ^^::) -> info {
            for (auto ns : inner_namespaces_of(in)) {
                if (identifier_of(ns) == name) return ns;
            }
            throw std::logic_error("namespace not found"); 
        };

        std::size_t c = 0;

        constexpr info tests_namespace = get_namespace_by_id("tests");

        constexpr auto [...test_suites] = [:reflect_constant_array(inner_namespaces_of(tests_namespace)):];

        template for (constexpr auto test_suite : {test_suites...}) {
            constexpr auto [...test_cases] = [:reflect_constant_array(members_of(test_suite, ctx)):];

            template for (constexpr auto test_case : {test_cases...}) {
                if constexpr (is_function(test_case)) {
                    all_tests[identifier_of(test_suite)].emplace_back(
                        identifier_of(test_case),
                        &[:test_case:]
                    );
                    c++;
                }
            }
        }

        return c;
    }

    // here where the imediatly invoked lambda get called in each Tu and push the tests in a global static map
    [[maybe_unused]] static size_t TU_tests_count = [](){ return collect_tests(); }();

    inline auto print_tests() -> void {
        for (auto const& [suite_id, tests_vec] : all_tests) {
            for (auto const& test : tests_vec) {
                std::cout << suite_id << '.' << std::get<0>(test) << std::endl;
            }
        }
    }

    inline auto run(std::string_view name, TestFuncType casefunc) -> bool {
        
        std::cout << name << " ... ";

        std::size_t failed = 0;
        current_yield = [&](const char* what, const char* file, int line) {
            failed++;
            if (failed == 1) {
                std::cout << COLOR_RED << "[failed]" << COLOR_RESET << std::endl;
            }
            std::cout << COLOR_YELLOW << "\t" << failed << ") -> " << what << " faild. " << COLOR_RESET << file << ":" << line << std::endl;
        };
    
        const auto t0 = std::chrono::steady_clock::now();

        casefunc();

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

        if (!failed) {
            std::cout << COLOR_GREEN << "[passed] " << '(' << ms << " ms" << ')' << COLOR_RESET << std::endl;
            return true;
        }

        std::cout << std::endl;
        return false;
    }

    inline auto run_test(std::string_view fulltestname) -> bool {

        auto pos = fulltestname.find('.');
        if (pos == std::string_view::npos)
            throw std::runtime_error("Invalid test format");

        auto tsuite_name = fulltestname.substr(0, pos);
        auto tcase_name  = fulltestname.substr(pos + 1);

        auto suite = all_tests.at(tsuite_name); //TODO: handle suite not exist
        auto it = std::ranges::find(suite, tcase_name, [](auto const& e){ return std::get<0>(e); });

        if (it != suite.end()) {
            return run(tcase_name, std::get<1>(*it));
        } else {
            std::cout << fulltestname << " not exist " << std::endl;
            return false;
        }
    }

    inline auto run_suite(std::string_view suite) -> bool {

        std::size_t failed = 0;

        for (auto const& [case_id, test_fn] : all_tests.at(suite)) {
            if(!run(case_id, test_fn)) failed++;
        }

        return failed == 0;
    }

    inline auto run_all() -> bool {

        std::size_t total_suites = 0;
        std::size_t total_cases  = 0;
        std::size_t total_failed = 0;

        std::cout << std::endl;
        
        for (auto const& [suite_id, tests_vec] : all_tests) {
            total_suites++;
            std::cout << COLOR_BOLD << total_suites << ") "  << suite_id << ' ' << COLOR_RESET << SEP << std::endl;

            for (auto const& [case_id, test_fn] : tests_vec) {
                std::cout << total_suites << '.' << ++total_cases << ") ";
                if(!run(case_id, test_fn)) total_failed++;
            }

            std::cout << std::endl;
        }

        // Summary footer
        std::cout << COLOR_BOLD << SEP << COLOR_RESET << std::endl;
        std::cout 
            << "   Suites: " << total_suites 
            << COLOR_GREEN << "   Successes: " << total_cases - total_failed << '/' << total_cases << COLOR_RESET
            << COLOR_RED << "   Failures: " << total_failed << '/' << total_cases << COLOR_RESET 
        << std::endl;
        std::cout << COLOR_BOLD << SEP << COLOR_RESET << std::endl;

        return total_failed == 0;
    }

    // calling main in one of your TUs only
    inline int main(int argc, char** argv) {
        if(argc > 1 && std::strcmp(argv[1], "--list") == 0){
            print_tests();
            return 0;
        } else if( argc > 1 && std::strcmp(argv[1], "--run") == 0) {
            if(!(argc > 2)) {
                std::cout << "provide testsuite.testcase" << std::endl;
                return 1;
            } else {
                if(run_test(std::string_view{argv[2]})) return 0;
                return 1;
            }
        } else if( argc > 1 && std::strcmp(argv[1], "--run-suite") == 0) {
            if(!(argc > 2)) {
                std::cout << "provide testsuite" << std::endl;

                return 1;
            } else {
                if(run_suite(std::string_view{argv[2]})) return 0;
                return 1;
            }
        } else {
            if(argc > 1){
                std::cout << "unknown option" << argv[1] << std::endl;

                return 1;
            }
        }

        if(!run_all()) return 1;
        else return 0;
    }


    #define expect_eq(x, y) do{if((x) != (y)) testing::current_yield("[ "#x" == "#y" ]", __FILE__, __LINE__); } while(false);
    #define expect_ne(x, y) do{if((x) == (y)) testing::current_yield("[ "#x" != "#y" ]", __FILE__, __LINE__); } while(false);
    #define expect_streq(x, y) do{if(std::strcmp(x, y) != 0) testing::current_yield("[ "#x" == "#y" ]", __FILE__, __LINE__); } while(false);
    #define expect_strne(x, y) do{if(std::strcmp(x, y) == 0) testing::current_yield("[ "#x" != "#y" ]", __FILE__, __LINE__); } while(false);
    #define expect_true(statment) do{if(!(statment)) testing::current_yield("[ "#statment" ]", __FILE__, __LINE__); } while(false);
    #define expect_false(statment) do{if((statment)) testing::current_yield("[ "#statment" ]", __FILE__, __LINE__); } while(false);
    #define expect_any_throw(statment) do{ static bool ____i__ = false; try { statment } catch(...) { ____i__= true; } \
                        if(!____i__) testing::current_yield("[ `"#statment"` not throwing ]", __FILE__, __LINE__); } while(false);
    #define expect_throw(statment, type) do{ static bool ____i__ = false; try { statment } catch(const type& e) { ____i__= true; } \
                        if(!____i__) testing::current_yield("[ `"#statment"` not throwing a `"#type"` ]", __FILE__, __LINE__); } while(false);
}

#ifdef TESTING_MAIN
int main(int argc, char** argv) { return testing::main(argc, argv); }
#endif