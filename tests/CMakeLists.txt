add_executable(csv_test
    csv_test.cpp
    $<$<BOOL:${CSVPP_ENABLE_CPP}>:cpp_test.cpp>
    $<$<BOOL:${CSVPP_ENABLE_C}>:c_test.cpp>
    $<$<BOOL:${CSVPP_ENABLE_EMBEDDED}>:embcsv_test.cpp>
    )
target_compile_features(csv_test PUBLIC cxx_std_17)
set_target_properties(csv_test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(csv_test PRIVATE -Wall -Wextra)
target_link_libraries(csv_test PUBLIC
    $<$<BOOL:${CSVPP_ENABLE_CPP}>:csvpp>
    $<$<BOOL:${CSVPP_ENABLE_C}>:csv>
    $<$<BOOL:${CSVPP_ENABLE_EMBEDDED}>:embcsv>
    )
target_compile_definitions(csv_test PRIVATE
    $<$<BOOL:${CSVPP_ENABLE_CPP}>:CSVPP_ENABLE_CPP>
    $<$<BOOL:${CSVPP_ENABLE_C}>:CSVPP_ENABLE_C>
    $<$<BOOL:${CSVPP_ENABLE_EMBEDDED}>:CSVPP_ENABLE_EMBEDDED>
    $<$<BOOL:${CSV_EMBCSV_NO_MALLOC}>:EMBCSV_NO_MALLOC>
    )

add_test(NAME csv_test COMMAND csv_test)
