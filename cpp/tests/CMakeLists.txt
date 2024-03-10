find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES_TESTS "./*.cpp")
add_executable(unit_tests ${SOURCES_TESTS})

enable_testing()

include(GoogleTest)
gtest_discover_tests(unit_tests)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gtest
    fmt::fmt
)
