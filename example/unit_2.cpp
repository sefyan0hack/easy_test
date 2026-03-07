#include "../testing.hpp"

auto my_div(int a, int b) -> int {
    return a / b;
}

// namespace as testsuite
namespace tests::divition {

    void my_div_random_tests() { // function as test case
        expect_eq(my_div(2, 2), 1);
        expect_ne(my_div(4, 2),2);

        //should fail
        expect_true(my_div(2, 2) == 5);
    }

    void my_div_hundred() {
        expect_eq(my_div(100, 2), 50);
        expect_eq(my_div(2, 100), 2/100);
    }

}
