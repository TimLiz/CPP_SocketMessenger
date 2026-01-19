project(Server)

add_executable(Server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        src/Network/Server/ClientConnection.cpp
        include/Network/Server/ClientConnection.h
        src/Services/ServerService.cpp
        include/Services/ServerService.h
        include/Services/PacketsDispatchService_/ServerPacketContext.h
        include/Services/PacketsDispatchService.h
)
target_include_directories(Server PRIVATE include)
target_link_libraries(Server PRIVATE Common)