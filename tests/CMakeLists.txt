FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.11.0
)

FetchContent_MakeAvailable(Catch2)

set(Link_Libs
    PRIVATE
        ITE_Libs
        Catch2::Catch2WithMain # Links the Catch2 implementation
)

add_executable(ite_test ite.tests.cpp)
target_link_libraries(ite_test ${Link_Libs})
add_test(NAME ite_test COMMAND ite_test)