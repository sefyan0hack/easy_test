#pragma once
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <meta>
#include <ranges>

namespace testing {
    namespace detail {
        using TestFuncType = void(void);

        inline std::unordered_map<
            std::string_view, // test suite
            std::vector<
                std::pair<
                    std::string_view, // test name
                    TestFuncType* // test function
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
                return info{};
            };

            std::size_t c = 0;

            constexpr info tests_namespace = get_namespace_by_id("tests");
            if constexpr (tests_namespace != info{}) {

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
            }

            return c;
        }

        // this only a hack for collecting all tests Across TUs `the lambda get called each time in each TU
        // and calling collect_tests<unique>() so there is TU time of collect_tests function
        // thanks to template<auto = [](){}> .
        [[maybe_unused]] static size_t TU_tests_count = [](){ return collect_tests(); }();

        auto current_yield(std::string what, const char* file, int line) -> void;

        inline std::string make_msg(const char* body) {
            return std::string("[ ") + body + " ]";
        }

        template<typename A, typename B>
        void check_eq(const A& a, const B& b, const char* expr, const char* file, int line) {
            if constexpr (std::is_convertible_v<A, std::string_view> &&
                        std::is_convertible_v<B, std::string_view>) {
                if (std::string_view(a) != std::string_view(b))
                    current_yield(make_msg(expr), file, line);
            } else {
                if (!(a == b))
                    current_yield(make_msg(expr), file, line);
            }
        }

        template<typename A, typename B>
        void check_ne(const A& a, const B& b, const char* expr, const char* file, int line) {
            if constexpr (std::is_convertible_v<A, std::string_view> &&
                        std::is_convertible_v<B, std::string_view>) {
                if (std::string_view(a) == std::string_view(b))
                    current_yield(make_msg(expr), file, line);
            } else {
                if (a == b)
                    current_yield(make_msg(expr), file, line);
            }
        }

        inline void check_true(bool cond, const char* expr, const char* file, int line) {
            if (!cond) current_yield(make_msg(expr), file, line);
        }

        template<typename F>
        void check_any_throw(F&& thunk, const char* expr, const char* file, int line) {
            bool ok = false;
            try { std::forward<F>(thunk)(); } catch(...) { ok = true; }
            if (!ok) current_yield(std::string("[ `") + expr + "` not throwing ]", file, line);
        }

        template<typename Ex, typename F>
        void check_throw(F&& thunk, const char* expr, const char* type_str, const char* file, int line) {
            bool ok = false;
            try { std::forward<F>(thunk)(); }
            catch (const Ex&) { ok = true; }
            catch (...) { /* other exceptions - not the expected type */ }
            if (!ok) current_yield(std::string("[ `") + expr + "` not throwing a `" + type_str + "` ]", file, line);
        }

    } // namespace detail

} // namespace testing

#define expect_eq(x,y)    testing::detail::check_eq((x),(y), #x " == " #y, __FILE__, __LINE__)
#define expect_ne(x,y)    testing::detail::check_ne((x),(y), #x " != " #y, __FILE__, __LINE__)
#define expect_streq(x,y) testing::detail::check_eq((x),(y), #x " == " #y, __FILE__, __LINE__)
#define expect_strne(x,y) testing::detail::check_ne((x),(y), #x " != " #y, __FILE__, __LINE__)
#define expect_true(e)    testing::detail::check_true(!!(e), #e, __FILE__, __LINE__)
#define expect_false(e)   testing::detail::check_true(!(e), "!(" #e ")", __FILE__, __LINE__)
#define expect_any_throw(stmt) testing::detail::check_any_throw([&](){ stmt; }, #stmt, __FILE__, __LINE__)
#define expect_throw(stmt, type) testing::detail::check_throw<type>([&](){ stmt; }, #stmt, #type, __FILE__, __LINE__)
