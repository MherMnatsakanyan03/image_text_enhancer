add_subdirectory(lib)

add_executable(ITE main.cpp)

# Copy resource files to the build directory after building the executable
add_custom_command(
    TARGET ITE POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
)

target_link_libraries(ITE PRIVATE ITE_Libs)

target_compile_options(ITE PRIVATE
    -O3
    -march=native
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE PRIVATE OpenMP::OpenMP_CXX)
endif()