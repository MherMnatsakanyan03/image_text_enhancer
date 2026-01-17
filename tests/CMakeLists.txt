FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)

FetchContent_MakeAvailable(Catch2)

set(Link_Libs
        PRIVATE
        ITE_Libs
        Catch2::Catch2WithMain # Links the Catch2 implementation
)

# --- Color tests ---
add_executable(grayscale_test color/ite.grayscale.tests.cpp)
target_link_libraries(grayscale_test ${Link_Libs})
add_test(NAME grayscale_test COMMAND grayscale_test)

add_executable(contrast_enhancement_test color/ite.contrast.tests.cpp)
target_link_libraries(contrast_enhancement_test ${Link_Libs})
add_test(NAME contrast_enhancement_test COMMAND contrast_enhancement_test)

add_executable(color_pass_test color/ite.color_pass.tests.cpp)
target_link_libraries(color_pass_test ${Link_Libs})
add_test(NAME color_pass_test COMMAND color_pass_test)


# --- Binarization tests ---
add_executable(binarization_test binarization/ite.binarize.tests.cpp)
target_link_libraries(binarization_test ${Link_Libs})
add_test(NAME binarization_test COMMAND binarization_test)


# --- Denoising tests ---
add_executable(simple_gaussian_test filters/ite.simple_gaussian.tests.cpp)
target_link_libraries(simple_gaussian_test ${Link_Libs})
add_test(NAME simple_gaussian_test COMMAND simple_gaussian_test)

add_executable(adaptive_gaussian_test filters/ite.adaptive_gaussian.tests.cpp)
target_link_libraries(adaptive_gaussian_test ${Link_Libs})
add_test(NAME adaptive_gaussian_test COMMAND adaptive_gaussian_test)

add_executable(adaptive_median_test filters/ite.adaptive_median.tests.cpp)
target_link_libraries(adaptive_median_test ${Link_Libs})
add_test(NAME adaptive_median_test COMMAND adaptive_median_test)

add_executable(despeckling_test filters/ite.despeckle.tests.cpp)
target_link_libraries(despeckling_test ${Link_Libs})
add_test(NAME despeckling_test COMMAND despeckling_test)


# --- Deskew tests ---
add_executable(deskew_test geometry/ite.deskew.tests.cpp)
target_link_libraries(deskew_test ${Link_Libs})
add_test(NAME deskew_test COMMAND deskew_test)


# --- Morphology tests ---
add_executable(dilation_test morphology/ite.dilation.tests.cpp)
target_link_libraries(dilation_test ${Link_Libs})
add_test(NAME dilation_test COMMAND dilation_test)

add_executable(erosion_test morphology/ite.erosion.tests.cpp)
target_link_libraries(erosion_test ${Link_Libs})
add_test(NAME erosion_test COMMAND erosion_test)