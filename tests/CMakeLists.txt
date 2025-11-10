FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.11.0
)

FetchContent_MakeAvailable(Catch2)

SET(Link_Libs
    PRIVATE
        ITE_Libs
        Catch2::Catch2WithMain # Links the Catch2 implementation
)

ADD_EXECUTABLE(dummy_test_exec DummyLibrary_Tests.cpp)

# Link the required libraries: DummyLibrary and Catch2's main library
TARGET_LINK_LIBRARIES(dummy_test_exec ${Link_Libs})

# Enable testing for the executable
ADD_TEST(NAME dummy_test COMMAND dummy_test_exec)