project(Client)

add_executable(Client ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        include/Services/PacketsDispatchService_/ClientPacketContext.h
        include/Services/PacketsDispatchService.h
)

target_include_directories(Client PRIVATE include)
target_link_libraries(Client PRIVATE Common)