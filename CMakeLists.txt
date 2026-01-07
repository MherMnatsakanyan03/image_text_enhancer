cmake_minimum_required(VERSION 3.28.0)
project(ITE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Manually specify JPEG paths for MinGW
set(JPEG_INCLUDE_DIR "C:/MinGW/include/libjpeg-turbo-gcc/include")
set(JPEG_LIBRARY     "C:/MinGW/include/libjpeg-turbo-gcc/lib/libjpeg.dll.a")
include_directories(${JPEG_INCLUDE_DIR})
link_directories("C:/MinGW/include/libjpeg-turbo-gcc/lib")

message(STATUS "Suche nach JPEG...")
message(STATUS "Benutze Include-Pfad: ${JPEG_INCLUDE_DIR}")
message(STATUS "Benutze Library-Pfad: ${JPEG_LIBRARY}")

find_package(OpenMP)
find_package(X11 REQUIRED)
find_package(JPEG REQUIRED)

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

add_compile_options(
  #-Wall # All warnings
  #-Wextra # Extra warnings
  #-Werror # Treat warnings as errors
)

include(FetchContent)

add_subdirectory(src)
add_subdirectory(tests)
