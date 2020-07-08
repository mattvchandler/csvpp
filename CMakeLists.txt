cmake_minimum_required (VERSION 3.8)
project(csv)

set(TITLE "CSV")
set(AUTHOR "Matthew Chandler <tardarsauce@gmail.com>")
set(COPYRIGHT "2020 Matthew Chandler")
set(SUMMARY "CSV parsing / writing library")
set(WEBSITE "http://github.com/mattvchandler/${PROJECT_NAME}")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

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

find_package(Doxygen)
if(DOXYGEN_FOUND)
    option(CSV_GEN_DOCS "Generate 'doc' target" ON)
    option(CSV_INTERNAL_DOCS "Generate documentation for private/internal methods, members, and functions" OFF)

    file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/doxygen_gen.cmake CONTENT
        "set(PROJECT_NAME \"${PROJECT_NAME}\")
        set(TITLE \"${TITLE}\")
        set(SUMMARY \"${SUMMARY}\")
        set(VERSION_SHORT \"${VERSION_SHORT}\")
        set(CMAKE_CURRENT_SOURCE_DIR  \"${CMAKE_CURRENT_SOURCE_DIR}\")
        set(CSV_INTERNAL_DOCS \"${CSV_INTERNAL_DOCS}\")

        set(CSV_DOC_INPUT_FILES \"$<$<BOOL:${CSV_ENABLE_CPP_CSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/csv.hpp\\\"> \\
                                  $<$<BOOL:${CSV_ENABLE_C_CSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/csv.h\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/csv.c\\\"> \\
                                  $<$<BOOL:${CSV_ENABLE_EMBCSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/embcsv.h\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/embcsv.c\\\">\")

        if(CSV_INTERNAL_DOCS)
            set(CSV_EXTRACT_INTERNAL_DOCS \"YES\")
            set(CSV_NO_EXTRACT_INTERNAL_DOCS \"NO\")
            set(CSV_DOC_ENABLED_SECTIONS \"INTERNAL\")
        else()
            set(CSV_EXTRACT_INTERNAL_DOCS \"NO\")
            set(CSV_NO_EXTRACT_INTERNAL_DOCS \"YES\")
            set(CSV_DOC_ENABLED_SECTIONS \"\")
        endif()

        set(CSV_DOC_PREDEFINED \"$<$<AND:$<BOOL:${CSV_ENABLE_EMBCSV}>,$<BOOL:${CSV_EMBCSV_NO_MALLOC}>>:EMBCSV_NO_MALLOC=1>\")

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            ${PROJECT_BINARY_DIR}/Doxyfile)
        ")

    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/doxygen_gen.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)

    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/doc/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        DEPENDS ${PROJECT_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/csv.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/csv.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/csv.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embcsv.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embcsv.h
        COMMENT "Generating documentation with Doxygen")

    add_custom_target(doc DEPENDS ${PROJECT_BINARY_DIR}/doc/html/index.html)

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES doc)
endif()
