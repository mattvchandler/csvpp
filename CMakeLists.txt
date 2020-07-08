cmake_minimum_required (VERSION 3.8...3.15)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(csv VERSION 1.0.0 LANGUAGES CXX C)

set(TITLE "CSV")
set(AUTHOR "Matthew Chandler <tardarsauce@gmail.com>")
set(COPYRIGHT "2020 Matthew Chandler")
set(SUMMARY "CSV parsing / writing library")
set(WEBSITE "http://github.com/mattvchandler/${PROJECT_NAME}")

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()


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

if(CSV_ENABLE_EMBCSV)
    option(CSV_EMBCSV_NO_MALLOC "Disable malloc in embedded CSV" OFF)
endif()

add_subdirectory(src)

if(CSV_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()

    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        option(CSV_GEN_DOCS "Generate 'doc' target" ON)
        option(CSV_INTERNAL_DOCS "Generate documentation for private/internal methods, members, and functions" OFF)

        file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/doxygen_gen.cmake CONTENT
            "set(PROJECT_NAME \"${PROJECT_NAME}\")
            set(TITLE \"${TITLE}\")
            set(SUMMARY \"${SUMMARY}\")
            set(PROJECT_VERSION \"${PROJECT_VERSION}\")
            set(CMAKE_CURRENT_SOURCE_DIR  \"${CMAKE_CURRENT_SOURCE_DIR}\")
            set(CSV_INTERNAL_DOCS \"${CSV_INTERNAL_DOCS}\")

            set(CSV_DOC_INPUT_FILES \"$<$<BOOL:${CSV_ENABLE_CPP_CSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/include/csv.hpp\\\"> \\
                                    $<$<BOOL:${CSV_ENABLE_C_CSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/include/csv.h\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/src/csv.c\\\"> \\
                                    $<$<BOOL:${CSV_ENABLE_EMBCSV}>:\\\"${CMAKE_CURRENT_SOURCE_DIR}/include/embcsv.h\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/src/embcsv.c\\\">\")

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
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/csv.h
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/csv.hpp
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/embcsv.h
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/csv.c
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/embcsv.c
            COMMENT "Generating documentation with Doxygen")

        add_custom_target(doc DEPENDS ${PROJECT_BINARY_DIR}/doc/html/index.html)

        set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES doc)
    endif()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    csvVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
    )
install(TARGETS csvpp csv embcsv
        EXPORT csvTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
        )

install(EXPORT csvTargets
    FILE csvConfig.cmake
        NAMESPACE csv::
        DESTINATION lib/cmake/csv
        )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/csvVersion.cmake"
        DESTINATION lib/cmake/csv)

# CPack vars
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_CONTACT "${AUTHOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY)
set(CPACK_PACKAGE_NAME "lib${PROJECT_NAME}-dev")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${TITLE}: ${SUMMARY}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_SOURCE_IGNORE_FILES
    ${PROJECT_BINARY_DIR}/*
    ${CMAKE_CURRENT_SOURCE_DIR}/.git/*
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}
    )

# Debian-specific settings
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_program(DPKG "dpkg")
    if(DPKG)
        set(CPACK_GENERATOR DEB;TGZ)
        execute_process(COMMAND ${DPKG} --print-architecture
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
        set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
        string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_DEBIAN_PACKAGE_NAME)
        set(CPACK_PACKAGE_FILE_NAME
            "${CPACK_DEBIAN_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    endif()
endif()

include(CPack)
