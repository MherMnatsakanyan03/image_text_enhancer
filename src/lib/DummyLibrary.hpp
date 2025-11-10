#pragma once
#include <string>

class DummyLibrary
{
public:
    // Constructor (does nothing)
    DummyLibrary() = default;

    /**
     * @brief Returns a fixed greeting string.
     * @return std::string "Hello from DummyLibrary!"
     */
    std::string getGreeting() const;

    /**
     * @brief A simple utility function for testing.
     * @param a First integer
     * @param b Second integer
     * @return int The sum of a and b
     */
    int add(int a, int b) const;
};