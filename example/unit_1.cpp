#define TESTING_MAIN
#include "../testing.hpp"

auto my_add(int a, int b) -> int {
    return a + b;
}

auto my_mul(int a, int b) -> int {
    return a * b;
}

// namespace as testsuite
namespace tests::addition {

    void my_add_random_tests() { // function as test case
        expect_eq(my_add(2, 2), 4);
        expect_ne(my_add(2, 2), 5);

        //should fail
        expect_strne("hh", "hh");
        expect_true(my_add(2, 2) == 5);
    }

    void my_add_hundred() {
        expect_eq(my_add(100, 2), 102);
        expect_eq(my_add(2, 100), 102);
    }

}

namespace tests::multiplication {

    void my_mul_number_by_1() {
        expect_eq(my_mul(1, 1), 1);

        //should fail
        expect_eq(my_mul(2, 1), 0);

    }
}

namespace tests::testingframework {

    void expect_eq_pass() {
        expect_eq(1, 1);
    }

    void expect_eq_fail() {
        expect_eq(1, 0);
    }

    void expect_ne_pass() {
        expect_ne(1, 0);
    }

    void expect_ne_fail() {
        expect_ne(1, 1);
    }

    void expect_streq_pass() {
        expect_streq("hello", "hello");
    }

    void expect_streq_fail() {
        expect_streq("hello", "bey");
    }

    void expect_strne_pass() {
        expect_strne("hello", "bey");
    }

    void expect_strne_fail() {
        expect_strne("hello", "hello");
    }

    void expect_true_pass() {
        expect_true(1 == 1);
    }

    void expect_true_fail() {
        expect_true(1 != 1);
    }

    void expect_false_pass() {
        expect_false(1 != 1);
    }

    void expect_false_fail() {
        expect_false(1 == 1);
    }

    void expect_any_throw_pass() {
        expect_any_throw( { throw 1; } );
    }

    void expect_any_throw_fail() {
        expect_any_throw( {int a = 0;} );
    }

    void expect_throw_pass() {
        expect_throw( { throw 1; }, int );
    }

    void expect_throw_fail() {
        expect_throw( {int a = 0;}, int );

    }

}