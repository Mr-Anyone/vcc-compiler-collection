add_executable(all_test lex.cpp stream.cpp)
target_link_libraries(all_test GTest::gtest_main comp)

file(GLOB resource_files "${CMAKE_CURRENT_SOURCE_DIR}/resource/*")
foreach(file_path IN LISTS resource_files)
    get_filename_component(file_name "${file_path}" NAME)
    configure_file("resource/${file_name}" "resource/${file_name}" COPYONLY)
endforeach()

include(GoogleTest)
gtest_discover_tests(all_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
