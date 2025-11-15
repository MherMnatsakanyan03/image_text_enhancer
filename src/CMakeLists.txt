add_subdirectory(lib)

add_executable(ITE main.cpp)

target_link_libraries(ITE PRIVATE ITE_Libs)

target_compile_options(ITE PRIVATE
    -O3
    -march=native
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE PRIVATE OpenMP::OpenMP_CXX)
endif()