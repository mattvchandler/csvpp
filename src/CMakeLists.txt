if(CSV_ENABLE_CPP_CSV)
    add_library(csvpp INTERFACE)
    add_library(csv::csvpp ALIAS csvpp)
    target_compile_features(csvpp INTERFACE cxx_std_17)
    target_include_directories(csvpp INTERFACE
                                     $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/>
                                     $<INSTALL_INTERFACE:include>
                                     )
    set_target_properties(csvpp PROPERTIES PUBLIC_HEADER ../include/csv.hpp)

    install(TARGETS csvpp
            EXPORT csvTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
            PUBLIC_HEADER DESTINATION include
            )
endif()

if(CSV_ENABLE_C_CSV)
    add_library(csv csv.c)
    add_library(csv::csv ALIAS csv)
    target_compile_features(csv PUBLIC c_std_11)
    set_target_properties(csv PROPERTIES C_EXTENSIONS OFF)
    target_compile_options(csv PRIVATE -Wall -Wextra)
    target_include_directories(csv PUBLIC
                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/>
                                   $<INSTALL_INTERFACE:include>
                                   )
    set_target_properties(csv PROPERTIES PUBLIC_HEADER ../include/csv.h)
    install(TARGETS csv
            EXPORT csvTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
            PUBLIC_HEADER DESTINATION include
            )
endif()

if(CSV_ENABLE_EMBCSV)
    add_library(embcsv embcsv.c)
    add_library(csv::embcsv ALIAS embcsv)
    target_compile_features(embcsv PUBLIC c_std_11)
    set_target_properties(embcsv PROPERTIES C_EXTENSIONS OFF)
    target_compile_options(embcsv PRIVATE -Wall -Wextra)
    target_include_directories(embcsv PUBLIC
                                      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/>
                                      $<INSTALL_INTERFACE:include>
                                      )
    set_target_properties(embcsv PROPERTIES PUBLIC_HEADER ../include/embcsv.h)
    install(TARGETS embcsv
            EXPORT csvTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
            PUBLIC_HEADER DESTINATION include
            )
endif()

install(EXPORT csvTargets
    FILE csvConfig.cmake
        NAMESPACE csv::
        DESTINATION lib/cmake/csv
        )
