cmake_minimum_required (VERSION 3.8)
project(csv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

option(CSV_ENABLE_CPP_CSV "Enable testing of c++ csv library" ON)
option(CSV_ENABLE_C_CSV "Enable generation and testing of c csv_library" OFF)
option(CSV_ENABLE_EMBCSV "Enable generation and testing of embedded csv library" OFF)
option(CSV_ENABLE_ALL "Enable generation and testing of all of the above" OFF)

option(CSV_ENABLE_EXAMPLES "Build example programs" OFF)

if(CSV_ENABLE_ALL)
    set(CSV_ENABLE_CPP_CSV ON)
    set(CSV_ENABLE_C_CSV ON)
    set(CSV_ENABLE_EMBCSV ON)
endif()

add_executable(csv_test)
target_sources(csv_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/csv_test.cpp)

if(CSV_ENABLE_CPP_CSV)
    target_sources(csv_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp_test.cpp)
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_CPP_CSV")
endif()

if(CSV_ENABLE_C_CSV)
    target_sources(csv_test PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/csv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/c_test.cpp
        )
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_C_CSV")
endif()

if(CSV_ENABLE_EMBCSV)
    target_sources(csv_test PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/embcsv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/embcsv_test.cpp
        )
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_EMBCSV")

    option(CSV_EMBCSV_NO_MALLOC "Disable malloc in embedded CSV" OFF)
    if(CSV_EMBCSV_NO_MALLOC)
        target_compile_definitions(csv_test PUBLIC "-DEMBCSV_NO_MALLOC")
    endif()
endif()

if(CSV_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()
