add_executable(all_test lex.cpp stream.cpp comp.cpp type.cpp)
target_link_libraries(all_test GTest::gtest_main comp)

file(GLOB resource_files "${CMAKE_CURRENT_SOURCE_DIR}/resource/*")
foreach(file_path IN LISTS resource_files)
    get_filename_component(file_name "${file_path}" NAME)
    configure_file("resource/${file_name}" "resource/${file_name}" COPYONLY)
endforeach()

include(GoogleTest)
gtest_discover_tests(all_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# FIXME: maybe we should just use a python script instead? 
# Find all test program files in test/test_program/
file(GLOB TEST_PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/program/*")

# Add a test for each file
foreach(TEST_PROGRAM ${TEST_PROGRAMS})
    # Extract just the filename (without full path)
    get_filename_component(TEST_NAME ${TEST_PROGRAM} NAME) # getting the test name

    add_test(
        NAME "test_compile_${TEST_NAME}"
        COMMAND ${CMAKE_BINARY_DIR}/src/vcc ${TEST_PROGRAM}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()
