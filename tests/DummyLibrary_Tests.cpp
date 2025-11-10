#include <catch2/catch_test_macros.hpp>
#include "DummyLibrary.hpp"

TEST_CASE("DummyLibrary Core Functionality", "[DummyLibrary]") {
    DummyLibrary dummy;

    SECTION("Greeting is correct") {
        // Act & Assert
        // Check if the getGreeting function returns the expected value
        REQUIRE(dummy.getGreeting() == "Hello from DummyLibrary!");
    }

    SECTION("Addition function works") {
        // Act & Assert
        // Check a simple calculation
        REQUIRE(dummy.add(5, 7) == 12);
        // Check addition with negative numbers
        REQUIRE(dummy.add(10, -5) == 5);
        // Check addition with zero
        REQUIRE(dummy.add(0, 1) == 1);
    }
}