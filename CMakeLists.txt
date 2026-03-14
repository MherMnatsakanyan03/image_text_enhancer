cmake_minimum_required(VERSION 3.28.0)
project(ITE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenMP)
find_package(JPEG REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

# Include CPM for dependency management, if the library uses a cmake-based approach
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

file(DOWNLOAD 
    https://raw.githubusercontent.com/dtschump/CImg/master/CImg.h 
    ${CMAKE_CURRENT_BINARY_DIR}/CImg/CImg.h
)

include(CTest)

if(MSVC)
    add_compile_options(
            /W4
            /WX
    )
else()
    add_compile_options(
            -Wall
            -Wextra
            -Werror
    )
endif()

include(FetchContent)

FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(cxxopts)

add_subdirectory(src)
add_subdirectory(tests)