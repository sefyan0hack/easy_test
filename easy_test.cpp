#include <easy_test/easy_test.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <spanstream>
#include <algorithm>
#include <functional>

namespace testing::detail {

    constexpr char const* COLOR_RED    = "\x1b[31m";
    constexpr char const* COLOR_GREEN  = "\x1b[32m";
    constexpr char const* COLOR_YELLOW = "\x1b[33m";
    constexpr char const* COLOR_BOLD   = "\x1b[1m";
    constexpr char const* COLOR_RESET  = "\x1b[0m";

    constexpr std::string_view SEP = "===================================================";

    thread_local std::vector<std::string> current_yield_fails;

    auto current_yield(std::string what, const char* file, int line) -> void
    {
        std::stringstream s;
        s << COLOR_YELLOW << "\t" << current_yield_fails.size() + 1 << " -> " << what << " faild. " << COLOR_RESET << file << ":" << line;
        current_yield_fails.push_back(s.str());
    }

    auto tests_list() -> std::vector<std::string> {
        return all_tests
            | std::views::transform([](auto const& suite_pair) {
                auto const& [suite_id, tests_vec] = suite_pair;
                return tests_vec
                    | std::views::transform([suite_id](auto const& test_pair) {
                        auto const& [test_id, _] = test_pair;
                        return std::string(suite_id) + std::string(".") + std::string(test_id);
                    });
            })
            | std::views::join
            | std::ranges::to<std::vector>();
    }

    template<class F, class... Args>
        requires std::invocable<F, Args...>
    auto execution_time_of(F&& func, Args&&... args) -> std::chrono::milliseconds
    {
        const auto t0 = std::chrono::steady_clock::now();
        std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0
        );
    }

    auto run(std::string_view name, TestFuncType* casefunc) -> bool {

        const auto ms = execution_time_of(casefunc);

        if(current_yield_fails.empty()) {
            std::cout << name << " ... " << COLOR_GREEN << "[passed] " << '(' << ms << ')' << COLOR_RESET << std::endl;
            return true;
        } else {
            std::cout << name << " ... " << COLOR_RED  << "[failed] " << '(' << ms << ')' << COLOR_RESET << std::endl;
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
        if (pos == std::string_view::npos) return false;

        auto tsuite_name = fulltestname.substr(0, pos);
        auto tcase_name  = fulltestname.substr(pos + 1);

        try {
            auto suite = all_tests.at(tsuite_name);
            auto it = std::ranges::find(suite, tcase_name, [](auto const& e){ return std::get<0>(e); });
            
            if (it != suite.end()) {
                return run(tcase_name, std::get<1>(*it));
            } else {
                std::cout << fulltestname << " not exist " << std::endl;
                return false;
            }
        } catch(...) {
            std::cout << "test case `" << tsuite_name << " not exist" << std::endl;
            return false;
        }
    }

    auto run_suite(std::string_view suite) -> bool {

        std::size_t failed = 0;

        try {
            for (auto const& [case_id, test_fn] : all_tests.at(suite)) {
                if(!run(case_id, test_fn)) failed++;
            }
        } catch(...) {
            std::cout << "suite `" << suite << "` not exist" << std::endl;
            return false;
        }

        return failed == 0;
    }

    auto run_all() -> bool {

        std::size_t total_suites = 0;
        std::size_t total_cases  = 0;
        std::size_t total_failed = 0;

        std::cout << std::endl;

        for (auto const& [suite_id, tests_vec] : all_tests) {
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

    struct help {
        char const* msg;
        template<std::size_t N> consteval help(const char (&m)[N]) : msg(std::define_static_string(m)) {}
    };

    template <class Opts>
    auto print_help(char** argv) -> void
    {
        using namespace std::meta;
        constexpr auto ctx = access_context::current();

        auto program_path = std::string(argv[0]);
        auto program_name = program_path.substr(program_path.find_last_of("/") + 1);

        std::cout << "Usage: " << program_name << " [options]\n";
        std::cout << "options:\n";

        // determine the longest option name (for alignment)
        std::size_t max_name_len = 0;
        template for (constexpr auto opt : std::define_static_array(nonstatic_data_members_of(^^Opts, ctx))) {
            std::string_view name = identifier_of(opt);
            if (name.size() > max_name_len) max_name_len = name.size();
        }

        const std::size_t desc_start = 4 + max_name_len + 2;

        template for (constexpr auto opt : std::define_static_array(nonstatic_data_members_of(^^Opts, ctx))) {
            std::string_view opt_name = identifier_of(opt);

            std::cout << "  --" << opt_name;

            std::size_t padding = desc_start - (4 + opt_name.size());
            std::cout << std::string(padding, ' ');

            bool first = true;
            template for (constexpr auto annot : std::define_static_array(annotations_of_with_type(opt, ^^help))) {
            std::string_view desc = extract<help>(annot).msg;
            if (first) {
                std::cout << desc << '\n';
                first = false;
            } else {
                std::cout << std::string(desc_start, ' ') << desc << '\n';
            }
            }
        }

        std::exit(EXIT_SUCCESS);
    }

    template <class Opts>
    auto cli_parse(int argc, char** argv, bool empty_opts_call_help = false) -> Opts {
        using namespace std::meta;

        std::vector<std::string_view> cmdline(argv+1, argv+argc);
        constexpr auto ctx = access_context::current();

        if(empty_opts_call_help && cmdline.empty()) print_help<Opts>(argv);
        if(std::ranges::contains(cmdline, std::string_view("--help"))) print_help<Opts>(argv);

        Opts opts;

        template for (constexpr auto opt : std::define_static_array(nonstatic_data_members_of(^^Opts, ctx))) {

            auto it = std::ranges::find_if(cmdline,
            [=](std::string_view arg){
                return (arg.starts_with("--") && arg.substr(2) == identifier_of(opt));
            });

            if(it != cmdline.end())
            {
            if (it + 1 == cmdline.end() || (*(it+1)).starts_with("--")) {
                if constexpr (type_of(opt) == ^^bool){
                opts.[:opt:] = true;
                continue;
                } else {
                std::cerr << "Missing value for option " <<  display_string_of(opt) << std::endl;
                std::exit(EXIT_FAILURE);
                }
            }

            auto iss = std::ispanstream(*(it+1));
            if (iss >> opts.[:opt:]; !iss) {
                std::cerr << "Failed to parse `" << *(it+1) << "` to type " <<  display_string_of(type_of(opt)) << std::endl;
                std::exit(EXIT_FAILURE);
            }
            }
        }
        return opts;
    }

    struct CliOptions {
        [[=help("show the list of existing tests `suite.case`.")]]
        bool list = false;

        [[=help("run testscases or tests suites.")]]
        [[=help("`suite.case` for runing test case.")]]
        [[=help("`suite` for runing all cases in suite.")]]
        [[=help(" or `*` for runing all default.")]]
        std::string run = "*";
    };
}

int main(int argc, char** argv) {
    using namespace testing::detail;
    auto opts = cli_parse<CliOptions>(argc, argv);

    if (opts.list) {
        for(auto id: tests_list()) std::cout << id << std::endl;
        return 0;
    }

    if (opts.run != "*") {
        if (opts.run.contains(".")){
            if(!run_test(opts.run)) return 1;
        } else {
            if(!run_suite(opts.run)) return 1;
        }
    } else {
        if(!run_all()) return 1;
    }

    return 0;
}