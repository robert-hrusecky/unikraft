#include <uk/diagnostic.h>

int test_func() {
    return 1234;
}


DIAGNOSTIC_FUNCTION("test", test_func);
