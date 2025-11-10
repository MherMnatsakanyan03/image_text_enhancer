#include <iostream>
#include "lib/DummyLibrary.hpp"

int main() {
    DummyLibrary dummy;

    // Use the library function
    std::cout << dummy.getGreeting() << std::endl;
    std::cout << "2 + 3 = " << dummy.add(2, 3) << std::endl;

    return 0;
}