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

# Grayscale tests
add_executable(grayscale_test ite.grayscale.tests.cpp)
target_link_libraries(grayscale_test ${Link_Libs})
add_test(NAME grayscale_test COMMAND grayscale_test)

# Binarization tests
add_executable(binarization_test ite.binarize.tests.cpp)
target_link_libraries(binarization_test ${Link_Libs})
add_test(NAME binarization_test COMMAND binarization_test)

# Denoising tests
add_executable(denoising_test ite.denoise.tests.cpp)
target_link_libraries(denoising_test ${Link_Libs})
add_test(NAME denoising_test COMMAND denoising_test)

# Deskew tests
add_executable(deskew_test ite.deskew.tests.cpp)
target_link_libraries(deskew_test ${Link_Libs})
add_test(NAME deskew_test COMMAND deskew_test)

# Dilation tests
add_executable(dilation_test ite.dilation.tests.cpp)
target_link_libraries(dilation_test ${Link_Libs})
add_test(NAME dilation_test COMMAND dilation_test)

# Erosion tests
add_executable(erosion_test ite.erosion.tests.cpp)
target_link_libraries(erosion_test ${Link_Libs})
add_test(NAME erosion_test COMMAND erosion_test)

# Contrast enhancement tests
add_executable(contrast_enhancement_test ite.contrast.tests.cpp)
target_link_libraries(contrast_enhancement_test ${Link_Libs})
add_test(NAME contrast_enhancement_test COMMAND contrast_enhancement_test)

# Despeckling tests
add_executable(despeckling_test ite.despeckle.tests.cpp)
target_link_libraries(despeckling_test ${Link_Libs})
add_test(NAME despeckling_test COMMAND despeckling_test)
