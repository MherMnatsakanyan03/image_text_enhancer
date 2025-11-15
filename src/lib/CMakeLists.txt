set(LIB_SRC
    ite.cpp
    ite.h
)

add_library(ITE_Libs STATIC ${LIB_SRC})


target_link_libraries(ITE_Libs PUBLIC
    ${X11_LIBRARIES} # For CImg display functions
    JPEG::JPEG       # For loading .jpg files
)
# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(ITE_Libs PUBLIC OpenMP::OpenMP_CXX)
endif()

target_compile_options(ITE_Libs PRIVATE
    -O3
)

target_compile_options(ITE_Libs PUBLIC    
    -Dcimg_use_jpeg
    -Wno-error=format-truncation
)

target_include_directories(ITE_Libs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/CImg
)