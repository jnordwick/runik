add_executable(tests test_reader.cpp)
target_link_libraries(tests runik_lib)
target_include_directories(tests PRIVATE ../external/ut/include)
add_test(NAME reader_tests COMMAND tests)
