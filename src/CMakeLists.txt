add_subdirectory(lib)

# Demo/Example executable (batch processes images from resources/)
add_executable(ITE_demo main.cpp)

# Copy resource files to the build directory after building the demo executable
add_custom_command(
    TARGET ITE_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
)

target_link_libraries(ITE_demo PRIVATE ITE_Libs)

target_compile_options(ITE_demo PRIVATE
    -O3
    -march=native
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE_demo PRIVATE OpenMP::OpenMP_CXX)
endif()

# CLI executable (command-line interface for single image processing)
add_executable(ITE_cli cli.cpp)

target_link_libraries(ITE_cli PRIVATE ITE_Libs)

target_compile_options(ITE_cli PRIVATE
    -O3
    -march=native
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE_cli PRIVATE OpenMP::OpenMP_CXX)
endif()

# Create a symbolic link or alias for the main CLI tool
# On install, ITE_cli will be the primary interface
set_target_properties(ITE_cli PROPERTIES OUTPUT_NAME "ite")
set_target_properties(ITE_demo PROPERTIES OUTPUT_NAME "ite-demo")
