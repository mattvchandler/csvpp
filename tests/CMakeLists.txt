add_executable(csv_test)
target_include_directories(csv_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../)
target_sources(csv_test PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/csv_test.cpp
    $<$<BOOL:${CSV_ENABLE_CPP_CSV}>:${CMAKE_CURRENT_LIST_DIR}/cpp_test.cpp>
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:${CMAKE_CURRENT_LIST_DIR}/c_test.cpp>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:${CMAKE_CURRENT_LIST_DIR}/embcsv_test.cpp>
    )
target_link_libraries(csv_test
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:csv>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:embcsv>
    )
target_compile_definitions(csv_test PUBLIC
    $<$<BOOL:${CSV_ENABLE_CPP_CSV}>:CSV_ENABLE_CPP_CSV>
    $<$<BOOL:${CSV_ENABLE_C_CSV}>:CSV_ENABLE_C_CSV>
    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:CSV_ENABLE_EMBCSV>
    $<$<BOOL:${CSV_EMBCSV_NO_MALLOC}>:EMBCSV_NO_MALLOC>
    )
