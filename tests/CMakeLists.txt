add_executable(csv_test
    csv_test.cpp
    $<$<BOOL:${CSV_ENABLE_CPP_CSV}>:cpp_test.cpp>
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:c_test.cpp>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:embcsv_test.cpp>
    )
target_compile_features(csv_test PUBLIC cxx_std_17)
set_target_properties(csv_test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(csv_test PRIVATE -Wall -Wextra)
target_link_libraries(csv_test PUBLIC
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:csv>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:embcsv>
    )
target_compile_definitions(csv_test PRIVATE
    $<$<BOOL:${CSV_ENABLE_CPP_CSV}>:CSV_ENABLE_CPP_CSV>
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:CSV_ENABLE_C_CSV>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:CSV_ENABLE_EMBCSV>
    $<$<BOOL:${CSV_EMBCSV_NO_MALLOC}>:EMBCSV_NO_MALLOC>
    )

add_test(NAME csv_test COMMAND csv_test)
