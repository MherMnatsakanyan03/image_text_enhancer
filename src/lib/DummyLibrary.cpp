#include "DummyLibrary.hpp"

std::string DummyLibrary::getGreeting() const {
    return "Hello from DummyLibrary!";
}

int DummyLibrary::add(int a, int b) const {
    return a + b;
}