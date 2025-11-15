set(LIB_SRC
    DummyLibrary.cpp
    DummyLibrary.hpp
)

add_library(ITE_Libs STATIC ${LIB_SRC})

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE_Libs PUBLIC OpenMP::OpenMP_CXX)
endif()

target_compile_options(ITE_Libs PRIVATE
    -O3
)

target_include_directories(ITE_Libs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})