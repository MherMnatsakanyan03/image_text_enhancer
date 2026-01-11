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

function(add_ite_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} ${Link_Libs})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Color tests
add_ite_test(grayscale_test color/ite.grayscale.tests.cpp)
add_ite_test(contrast_enhancement_test color/ite.contrast.tests.cpp)

# Binarization tests
add_ite_test(binarization_test binarization/ite.binarize.tests.cpp)

# Denoising tests
add_ite_test(simple_gaussian_test filters/ite.simple_gaussian.tests.cpp)
add_ite_test(adaptive_gaussian_test filters/ite.adaptive_gaussian.tests.cpp)
add_ite_test(adaptive_median_test filters/ite.adaptive_median.tests.cpp)
add_ite_test(despeckling_test filters/ite.despeckle.tests.cpp)

# Deskew tests
add_ite_test(deskew_test geometry/ite.deskew.tests.cpp)

# Morphology tests
add_ite_test(dilation_test morphology/ite.dilation.tests.cpp)
add_ite_test(erosion_test morphology/ite.erosion.tests.cpp)
