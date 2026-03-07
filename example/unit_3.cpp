#include "../testing.hpp"

auto my_abs(int a) -> int {
    return a >= 0 ? a : -a;
}

namespace tests::absolute {

    void my_abs_random_tests() {
        expect_eq(my_abs(2), 2);
        expect_ne(my_abs(4), -4);

        //should fail
        expect_ne(my_abs(-4), 4);
        expect_true(my_abs(-1) != 1);
    }

}
