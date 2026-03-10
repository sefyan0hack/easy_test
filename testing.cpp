#include "testing.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

namespace {
    constexpr char const* COLOR_RED    = "\x1b[31m";
    constexpr char const* COLOR_GREEN  = "\x1b[32m";
    constexpr char const* COLOR_YELLOW = "\x1b[33m";
    constexpr char const* COLOR_BOLD   = "\x1b[1m";
    constexpr char const* COLOR_RESET  = "\x1b[0m";

    constexpr std::string_view SEP = "===================================================";

    std::vector<std::string> current_yield_fails;

    auto print_tests() -> void {
        for (auto const& [suite_id, tests_vec] : testing::all_tests) {
            for (auto const& [test_id, func] : tests_vec) {
                std::cout << suite_id << '.' << test_id << std::endl;
            }
        }
    }

    auto run(std::string_view name, testing::TestFuncType* casefunc) -> bool {
    
        const auto t0 = std::chrono::steady_clock::now();

        casefunc();

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

        if(current_yield_fails.empty()) {
            std::cout << name << " ... " << COLOR_GREEN << "[passed] " << '(' << ms << " ms" << ')' << COLOR_RESET << std::endl;
            return true;
        } else {
            std::cout << name << " ... " << COLOR_RED  << "[failed] " << '(' << ms << " ms" << ')' << COLOR_RESET << std::endl;
            for(const auto& e : current_yield_fails){
                std::cout << e << std::endl;
            }
            std::cout << std::endl;

            current_yield_fails.clear();
            return false;
        }
    }

    auto run_test(std::string_view fulltestname) -> bool {

        auto pos = fulltestname.find('.');
        if (pos == std::string_view::npos)
            throw std::runtime_error("Invalid test format");

        auto tsuite_name = fulltestname.substr(0, pos);
        auto tcase_name  = fulltestname.substr(pos + 1);

        auto suite = testing::all_tests.at(tsuite_name); //TODO: handle suite not exist
        auto it = std::ranges::find(suite, tcase_name, [](auto const& e){ return std::get<0>(e); });

        if (it != suite.end()) {
            return run(tcase_name, std::get<1>(*it));
        } else {
            std::cout << fulltestname << " not exist " << std::endl;
            return false;
        }
    }

    auto run_suite(std::string_view suite) -> bool {

        std::size_t failed = 0;

        for (auto const& [case_id, test_fn] : testing::all_tests.at(suite)) {
            if(!run(case_id, test_fn)) failed++;
        }

        return failed == 0;
    }

    auto run_all() -> bool {

        std::size_t total_suites = 0;
        std::size_t total_cases  = 0;
        std::size_t total_failed = 0;

        std::cout << std::endl;
        
        for (auto const& [suite_id, tests_vec] : testing::all_tests) {
            total_suites++;
            std::cout << COLOR_BOLD << total_suites << ") "  << suite_id << ' ' << COLOR_RESET << SEP << std::endl;

            for (auto const& [case_id, test_fn] : tests_vec) {
                std::cout << "  " << total_suites << '.' << ++total_cases << ") ";
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
    
}

auto testing::current_yield(const char* what, const char* file, int line) -> void
{
    std::stringstream s;
    s << COLOR_YELLOW << "\t" << current_yield_fails.size() + 1 << " -> " << what << " faild. " << COLOR_RESET << file << ":" << line;
    current_yield_fails.push_back(s.str());
}


int main(int argc, char** argv) {
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