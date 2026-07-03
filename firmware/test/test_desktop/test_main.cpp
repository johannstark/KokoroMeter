#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_placeholder(void) {
    // A simple test to ensure the native test environment compiles and runs
    TEST_ASSERT_EQUAL(32, 32);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_placeholder);
    UNITY_END();

    return 0;
}
