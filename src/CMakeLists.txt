if(CSVPP_ENABLE_CPP)
    add_library(csvpp INTERFACE)
    add_library(csvpp::csvpp ALIAS csvpp)
    target_compile_features(csvpp INTERFACE cxx_std_17)
    target_include_directories(csvpp INTERFACE
                                     $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
                                     $<INSTALL_INTERFACE:include>
                                     )
    set_target_properties(csvpp PROPERTIES PUBLIC_HEADER ../include/csvpp/csv.hpp)

    install(TARGETS csvpp
            EXPORT csvTargets
            LIBRARY DESTINATION lib/csvpp
            ARCHIVE DESTINATION lib/csvpp
            RUNTIME DESTINATION bin/csvpp
            PUBLIC_HEADER DESTINATION include/csvpp
            )

        configure_file(${CMAKE_CURRENT_LIST_DIR}/../${PROJECT_NAME}.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}.pc" DESTINATION "share/pkgconfig")
endif()

if(CSVPP_ENABLE_C)
    add_library(csv csv.c)
    add_library(csvpp::csv ALIAS csv)
    target_compile_features(csv PUBLIC c_std_11)
    set_target_properties(csv PROPERTIES C_EXTENSIONS OFF)
    target_compile_options(csv PRIVATE -Wall -Wextra)
    target_include_directories(csv PUBLIC
                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
                                   $<INSTALL_INTERFACE:include>
                                   )
    set_target_properties(csv PROPERTIES PUBLIC_HEADER ../include/csvpp/csv.h)
    install(TARGETS csv
            EXPORT csvTargets
            LIBRARY DESTINATION lib/csvpp
            ARCHIVE DESTINATION lib/csvpp
            RUNTIME DESTINATION bin/csvpp
            PUBLIC_HEADER DESTINATION include/csvpp
            )

        configure_file(${CMAKE_CURRENT_LIST_DIR}/../${PROJECT_NAME}-c.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}-c.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}-c.pc" DESTINATION "share/pkgconfig")
endif()

if(CSVPP_ENABLE_EMBEDDED)
    add_library(embcsv embcsv.c)
    add_library(csvpp::embcsv ALIAS embcsv)
    target_compile_features(embcsv PUBLIC c_std_11)
    set_target_properties(embcsv PROPERTIES C_EXTENSIONS OFF)
    target_compile_options(embcsv PRIVATE -Wall -Wextra)
    target_include_directories(embcsv PUBLIC
                                      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
                                      $<INSTALL_INTERFACE:include>
                                      )
    set_target_properties(embcsv PROPERTIES PUBLIC_HEADER ../include/csvpp/embcsv.h)
    install(TARGETS embcsv
            EXPORT csvTargets
            LIBRARY DESTINATION lib/csvpp
            ARCHIVE DESTINATION lib/csvpp
            RUNTIME DESTINATION bin/csvpp
            PUBLIC_HEADER DESTINATION include/csvpp
            )

        configure_file(${CMAKE_CURRENT_LIST_DIR}/../${PROJECT_NAME}-embcsv.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}-embcsv.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}-embcsv.pc" DESTINATION "share/pkgconfig")
endif()

install(EXPORT csvTargets
    FILE csvConfig.cmake
        NAMESPACE csvpp::
        DESTINATION lib/cmake/csvpp
        )
