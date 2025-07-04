add_executable(lex lex.cpp)
target_link_libraries(lex GTest::gtest_main comp)

include(GoogleTest)
gtest_discover_tests(lex)
