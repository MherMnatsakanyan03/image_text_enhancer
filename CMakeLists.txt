CMAKE_MINIMUM_REQUIRED(VERSION 3.28.0)
project(MyCppBoilerplate LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

FIND_PACKAGE(OpenMP)

INCLUDE(CTest)

ADD_COMPILE_OPTIONS(
    -Wall        # All warnings
    -Wextra      # Extra warnings
    -Werror      # Treat warnings as errors
)

INCLUDE(FetchContent)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)