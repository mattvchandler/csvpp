cmake_minimum_required (VERSION 3.14.2...3.18)

project(csvpp VERSION 1.0.0 LANGUAGES CXX C)
set(PROJECT_DESCRIPTION "CSV parsing / writing libraries")

set(AUTHOR "Matthew Chandler <tardarsauce@gmail.com>")

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

option(CSVPP_ENABLE_CPP "Enable testing of c++ csv library" ON)
option(CSVPP_ENABLE_C "Enable generation and testing of c csv_library" OFF)
option(CSVPP_ENABLE_EMBEDDED "Enable generation and testing of embedded csv library" OFF)
option(CSVPP_ENABLE_ALL "Enable generation and testing of all of the above" OFF)

if(CSVPP_ENABLE_ALL)
    set(CSVPP_ENABLE_CPP ON)
    set(CSVPP_ENABLE_C ON)
    set(CSVPP_ENABLE_EMBEDDED ON)
endif()

if(CSVPP_ENABLE_EMBEDDED)
    option(CSVPP_EMBEDDED_NO_MALLOC "Disable malloc in embedded CSV" OFF)
endif()

add_subdirectory(src)

option(CSVPP_ENABLE_EXAMPLES "Build example programs" OFF)
if(CSVPP_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "" OFF)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()

    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        option(CSVPP_INTERNAL_DOCS "Generate documentation for private/internal methods, members, and functions" OFF)

        set(CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
        if(CSVPP_ENABLE_CPP)
            list(APPEND CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/include/csvpp/csv.hpp")
        endif()
        if(CSVPP_ENABLE_C)
            list(APPEND CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/include/csvpp/csv.h")
            if(CSVPP_INTERNAL_DOCS)
                list(APPEND CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/csv.c")
            endif()
        endif()
        if(CSVPP_ENABLE_EMBEDDED)
            list(APPEND CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/include/csvpp/embcsv.h")
            if(CSVPP_INTERNAL_DOCS)
                list(APPEND CSVPP_DOXYGEN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/embcsv.c")
            endif()
            if(CSV_EMBCSV_NO_MALLOC)
                set(DOXYGEN_PREDEFINED "EMBCSV_NO_MALLOC=1")
            endif()
        endif()

        if(CSVPP_INTERNAL_DOCS)
            set(DOXYGEN_EXTRACT_PRIVATE "YES")
            set(DOXYGEN_PRIV_VIRTUAL "YES")
            set(DOXYGEN_EXTRACT_PACKAGE "YES")
            set(DOXYGEN_INTERNAL_DOCS "YES")
            set(DOXYGEN_ENABLED_SECTIONS "INTERNAL")
        else()
            set(DOXYGEN_EXCLUDE_SYMBOLS "detail")
        endif()

        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

        doxygen_add_docs(doc ${CSVPP_DOXYGEN_SRCS})
    endif()

    add_subdirectory(pkgconfig)

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        csvppConfigVersion.cmake
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
        )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/csvppConfigVersion.cmake"
            DESTINATION lib/cmake/csvpp)

    install(EXPORT csvTargets
            FILE csvppConfig.cmake
            NAMESPACE csvpp::
            DESTINATION lib/cmake/csvpp
            )

    # CPack vars
    set(CPACK_GENERATOR TGZ)
    set(CPACK_PACKAGE_CONTACT "${AUTHOR}")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-dev")
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
        ${CMAKE_CURRENT_BINARY_DIR}/*
        ${CMAKE_CURRENT_SOURCE_DIR}/.git/*
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
endif()
