#pragma once
#include <functional>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <meta>
#include <ranges>
#include <tuple>

//TODO: time testcases
//TODO: improve formatting 
//TODO: maybe add  more features

namespace tests {}

namespace testing_framework {

    constexpr char const* COLOR_RED    = "\x1b[31m";
    constexpr char const* COLOR_GREEN  = "\x1b[32m";
    constexpr char const* COLOR_YELLOW = "\x1b[33m";
    constexpr char const* COLOR_BOLD   = "\x1b[1m";
    constexpr char const* COLOR_RESET  = "\x1b[0m";

    constexpr std::string_view SEP = "========================================";

    inline std::function<void(const char*)> current_yield = nullptr;
    using TestFuncType = std::function<void(void)>;

    // nice c++ type system :)
    inline std::unordered_map<
        std::string_view, // test suite
        std::vector<
            std::tuple<
                std::string_view, // test name
                TestFuncType // test function
            >
        >
    > all_tests;

    template<auto = [](){}>
    inline auto collect_tests() -> size_t {
        using namespace std::meta;
        constexpr auto ctx = access_context::unprivileged();

        constexpr auto inner_namespaces = [](info ns) {
            return members_of(ns, ctx)
                | std::views::filter(is_namespace)
                | std::ranges::to<std::vector>();
        };
    
        std::size_t c = 0;
        template for (constexpr auto test_suite : [:reflect_constant_array(inner_namespaces(^^::tests)):]) {
            template for (constexpr auto test_case : [:reflect_constant_array(members_of(test_suite, ctx)):]) {
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
                auto const& case_id = std::get<0>(test);

                std::printf("%.*s.%.*s\n",
                    static_cast<int>(suite_id.length()), suite_id.data(),
                    static_cast<int>(case_id.length()), case_id.data()
                );
            }
        }
    }

    inline auto run(std::string_view name, TestFuncType casefunc) -> bool {

        std::printf("%.*s ... ",
            static_cast<int>(name.length()), name.data()
        );

        std::size_t failed = 0;
        current_yield = [&](const char* e) {
            failed++;
            if (failed == 1) {
                std::printf("%s[failed]%s\n", COLOR_RED, COLOR_RESET);
            }
            std::printf("\t%s%zu) %s%s\n", COLOR_YELLOW, failed, e, COLOR_RESET);
        };

        casefunc();

        if (!failed) {
            std::printf("%s[passed]%s\n", COLOR_GREEN, COLOR_RESET);
            return true;
        }

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
            std::printf("`%.*s` not exist ",
                static_cast<int>(fulltestname.length()), fulltestname.data()
            );
            return false;
        }
    }

    inline auto run_suite(std::string_view suite) -> bool {

        std::size_t failed = 0;

        for (auto const& test : all_tests.at(suite)) {
            auto const& case_id = std::get<0>(test);
            auto const& test_fn = std::get<1>(test);

            if(!run(case_id, test_fn)) failed++;
        }
        return failed == 0;
    }

    inline auto run_all() -> bool {

        std::puts("");
        std::printf("%s%s%s\n", COLOR_BOLD, SEP.data(), COLOR_RESET);
        std::printf("%s  UNIT TESTS  %s\n", COLOR_BOLD, COLOR_RESET);
        std::printf("%s%s%s\n\n", COLOR_BOLD, SEP.data(), COLOR_RESET);

        std::size_t total_suites = 0;
        std::size_t total_cases  = 0;
        std::size_t total_failed = 0;
        
        for (auto const& [suite_id, tests_vec] : all_tests) {
            total_suites++;
            std::printf("%s%zu) %.*s%s\n",
                        COLOR_BOLD,
                        total_suites,
                        static_cast<int>(suite_id.length()), suite_id.data(),
                        COLOR_RESET);

            for (auto const& test : tests_vec) {
                auto const& case_id = std::get<0>(test);
                auto const& test_fn = std::get<1>(test);

                total_cases++;
                std::printf("  %zu.%zu) ", total_suites, total_cases);
                if(!run(case_id, test_fn)) total_failed++;
            }

            std::puts("");
        }

        // Summary footer
        std::printf("%s%s%s\n", COLOR_BOLD, SEP.data(), COLOR_RESET);
        const char* fail_color = total_failed ? COLOR_RED : COLOR_GREEN;
        std::printf("Suites: %zu   Cases: %zu   %sFailures: %zu%s\n",
                    total_suites,
                    total_cases,
                    fail_color,
                    total_failed,
                    COLOR_RESET);
        std::printf("%s%s%s\n", COLOR_BOLD, SEP.data(), COLOR_RESET);

        return total_failed == 0;
    }

    // calling main in one of your TUs only
    inline int main(int argc, char** argv) {
        if(argc > 1 && std::strcmp(argv[1], "--list") == 0){
            print_tests();
            return 0;
        } else if( argc > 1 && std::strcmp(argv[1], "--run") == 0) {
            if(!(argc > 2)) {
                std::printf( "provide testsuite.testcase");
                return 1;
            } else {
                if(run_test(std::string_view{argv[2]})) return 0;
                return 1;
            }
        } else if( argc > 1 && std::strcmp(argv[1], "--run-suite") == 0) {
            if(!(argc > 2)) {
                std::printf( "provide testsuite");
                return 1;
            } else {
                if(run_suite(std::string_view{argv[2]})) return 0;
                return 1;
            }
        } else {
            if(argc > 1){
                std::printf("unknown option %s", argv[1]); 
                return 1;
            }
        }

        if(!run_all()) return 1;
    }

    #define STRINGIFY(x) #x
    #define TOSTRING(x) STRINGIFY(x)
    #define expect_eq(x, y) do{if((x) != (y)) testing_framework::current_yield( "-> [ "#x" == "#y" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_ne(x, y) do{if((x) == (y)) testing_framework::current_yield( "-> [ "#x" != "#y" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_streq(x, y) do{if(std::strcmp(x, y) != 0) testing_framework::current_yield( "-> [ "#x" == "#y" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_strne(x, y) do{if(std::strcmp(x, y) == 0) testing_framework::current_yield( "-> [ "#x" != "#y" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_true(statment) do{if(!(statment)) testing_framework::current_yield( "-> [ "#statment" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_false(statment) do{if((statment)) testing_framework::current_yield( "-> [ "#statment" ] failed. " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_any_throw(statment) do{ static bool ____i__ = false; try { statment } catch(...) { ____i__= true; } \
                        if(!____i__) testing_framework::current_yield( "-> [ `"#statment"` not throwing ]  " __FILE__ ":" TOSTRING(__LINE__)); } while(false);
    #define expect_throw(statment, type) do{ static bool ____i__ = false; try { statment } catch(const type& e) { ____i__= true; } \
                        if(!____i__) testing_framework::current_yield( "-> [ `"#statment"` not throwing a `"#type"` ]  " __FILE__ ":" TOSTRING(__LINE__)); } while(false);

}

#ifdef TESTING_MAIN
int main(int argc, char** argv) { return testing_framework::main(argc, argv); }
#endif