include("${CMAKE_SOURCE_DIR}/tools/secTransCert.cmake")

project(Server)

add_executable(Server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        src/Network/Server/ClientConnection.cpp
        include/Network/Server/ClientConnection.h
        src/Services/ServerService.cpp
        include/Services/ServerService.h
        include/Services/PacketsDispatchService_/ServerPacketContext.h
        include/Services/PacketsDispatchService.h
)

if (ENABLE_SECURE_TRANSPORT)
    add_dependencies(Server sectrans_certs)

    add_custom_command(TARGET Server POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:Server>/RSA"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SECTRANS_PRIVKEY}"
            "$<TARGET_FILE_DIR:Server>/RSA"
            COMMENT "Copying server private key into server output folder"
    )
endif()

target_include_directories(Server PRIVATE include)
target_link_libraries(Server PRIVATE Common)