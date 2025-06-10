set(
    SOURCE_FILES
    driver.cpp
    lex.cpp
)

add_executable(
    comp
    ${SOURCE_FILES}
)

target_include_directories(
    comp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(comp PRIVATE -Wall -Werror -Wextra)
