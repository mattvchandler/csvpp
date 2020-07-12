if(CSVPP_ENABLE_CPP)
        configure_file(${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION "share/pkgconfig")
endif()

if(CSVPP_ENABLE_C)
        configure_file(${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-c.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-c.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-c.pc" DESTINATION "share/pkgconfig")
endif()

if(CSVPP_ENABLE_EMBEDDED)
        configure_file(${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-embcsv.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-embcsv.pc
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-embcsv.pc" DESTINATION "share/pkgconfig")
endif()
