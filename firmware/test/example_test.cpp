#include <unity.h>

// A simple function we want to test
int compute_sum(int a, int b) { return a + b; }

void setUp(void) {
    // Optional: Runs before every single test case
}

void tearDown(void) {
    // Optional: Runs after every single test case
}

void test_math_addition_should_pass(void) {
    TEST_ASSERT_EQUAL_INT(30, compute_sum(10, 20));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_math_addition_should_pass);

    return UNITY_END();
}
