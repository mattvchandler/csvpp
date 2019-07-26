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
option(CSV_ENABLE_TINYCSV "Enable generation and testing of tinycsv" OFF)
option(CSV_ENABLE_LIBCSV "Enable testing against libcsv" OFF)
option(CSV_ENABLE_PYTHON "Enable testing against python's csv libary" OFF)

add_executable(csv_test)
target_sources(csv_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/csv_test.cpp)

if(CSV_ENABLE_CPP_CSV)
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_CPP_CSV")
endif()

if(CSV_ENABLE_C_CSV)
    target_sources(csv_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/csv.c)
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_C_CSV")
endif()

if(CSV_ENABLE_EMBCSV)
    target_sources(csv_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/embcsv.c)
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_EMBCSV")
endif()

if(CSV_ENABLE_TINYCSV)
    file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/tinycsv_test.cmake CONTENT
        "set(CMAKE_CURRENT_SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")
        set(PROJECT_BINARY_DIR \"${PROJECT_BINARY_DIR}\")
        file(READ \${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.cpp tinycsv_cpp)
        string(REGEX REPLACE \"p\\\\(\" \"tinycsv_parse(\" tinycsv_cpp \"\${tinycsv_cpp}\")
        string(REGEX REPLACE \"main\" \"tinycsv_disabled_main\" tinycsv_cpp \"\${tinycsv_cpp}\")
        file(WRITE \${PROJECT_BINARY_DIR}/tinycsv_parse.cpp \"\${tinycsv_cpp}\")\n")
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/tinycsv_test.cmake
        OUTPUT ${PROJECT_BINARY_DIR}/tinycsv_parse.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.cpp
        COMMENT "Extacting parse fun from tinycsv.cpp")

    file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/tinycsv_expanded_test.cmake CONTENT
        "set(CMAKE_CURRENT_SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")
        set(PROJECT_BINARY_DIR \"${PROJECT_BINARY_DIR}\")
        file(READ \${CMAKE_CURRENT_SOURCE_DIR}/tinycsv_expanded.cpp tinycsv_expanded_cpp)
        string(REGEX REPLACE \"parse\\\\(\" \"tinycsv_expanded_parse(\" tinycsv_expanded_cpp \"\${tinycsv_expanded_cpp}\")
        string(REGEX REPLACE \"main\" \"tinycsv_expanded_disabled_main\" tinycsv_expanded_cpp \"\${tinycsv_expanded_cpp}\")
        file(WRITE \${PROJECT_BINARY_DIR}/tinycsv_expanded_parse.cpp \"\${tinycsv_expanded_cpp}\")\n")
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/tinycsv_expanded_test.cmake
        OUTPUT ${PROJECT_BINARY_DIR}/tinycsv_expanded_parse.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv_expanded.cpp
        COMMENT "Extacting parse fun from tinycsv_expanded.cpp")

    add_executable(tinycsv tinycsv.cpp)
    add_executable(tinycsv_expanded tinycsv_expanded.cpp)

    find_program(qrencode qrencode)
    if(qrencode)
        add_custom_command(
            COMMAND ${qrencode} -r ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.cpp -o ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.png
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.png
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.cpp
            COMMENT "Generating tinycsv QR code")
        add_custom_target(tinycsv_qr ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tinycsv.png)
    endif()

    target_sources(csv_test PUBLIC
        ${PROJECT_BINARY_DIR}/tinycsv_parse.cpp
        ${PROJECT_BINARY_DIR}/tinycsv_expanded_parse.cpp
        )

    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_TINYCSV")
endif()

if(CSV_ENABLE_LIBCSV)
    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_LIBCSV")
    target_link_libraries(csv_test csv)
endif()

if(CSV_ENABLE_PYTHON)
    find_package(pybind11 REQUIRED)
    include_directories(pybind11test ${PYBIND11_INCLUDE_DIRS})

    target_compile_definitions(csv_test PUBLIC "-DCSV_ENABLE_PYTHON")
    target_link_libraries(csv_test pybind11::embed)
endif()
