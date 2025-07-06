add_executable(all_test lex.cpp stream.cpp)
target_link_libraries(all_test GTest::gtest_main comp)

# TODO: fixme please
configure_file(resource/streamtest.txt resource/streamtest.txt
    COPYONLY)

configure_file(resource/new_line_loc_test.txt resource/new_line_loc_test.txt
    COPYONLY)

include(GoogleTest)
gtest_discover_tests(all_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
