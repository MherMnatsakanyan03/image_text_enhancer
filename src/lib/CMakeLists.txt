# Main library source files
set(LIB_SRC
        # Facade
        ite.cpp
        ite.h

        # Core utilities
        core/integral_image.cpp
        core/integral_image.h
        core/utils.h

        # Color operations
        color/grayscale.cpp
        color/grayscale.h
        color/contrast.cpp
        color/contrast.h
        color/color.cpp
        color/color.h

        # Binarization
        binarization/binarization.cpp
        binarization/binarization.h

        # Morphological operations
        morphology/morphology.cpp
        morphology/morphology.h

        # Geometric transformations
        geometry/geometry.cpp
        geometry/geometry.h

        # Filters
        filters/filters.cpp
        filters/filters.h

        # I/O
        io/image_io.cpp
        io/image_io.h
)

add_library(ITE_Libs STATIC ${LIB_SRC})

target_link_libraries(ITE_Libs PUBLIC
        JPEG::JPEG
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ITE_Libs PUBLIC
            ${X11_LIBRARIES}
    )
endif()

# Link OpenMP if found
if (OpenMP_CXX_FOUND)
    target_link_libraries(ITE_Libs PUBLIC OpenMP::OpenMP_CXX)
endif ()

if(MSVC)
    target_compile_options(ITE_Libs PRIVATE
            /O2
    )
else()
    target_compile_options(ITE_Libs PRIVATE
            -O3
    )

    target_compile_options(ITE_Libs PUBLIC
            -Wno-error=format-truncation
    )
endif()

target_compile_definitions(ITE_Libs PUBLIC
        cimg_use_jpeg
)

target_include_directories(ITE_Libs PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/CImg
)