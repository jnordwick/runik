function(runik_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} runik_lib)
  target_include_directories(${name} PRIVATE ../external/ut/include)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

runik_test(test_reader)
runik_test(test_vec)
