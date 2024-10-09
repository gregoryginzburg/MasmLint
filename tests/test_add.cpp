#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "add.h"

// int add(int a, int b) { return a + b; }

TEST_CASE("testing the add function") {
    CHECK(add(1, 2) == 3);
    CHECK(add(2, 5) == 7);
}