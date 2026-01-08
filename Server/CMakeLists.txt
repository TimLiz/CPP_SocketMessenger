project(Server)

set(CMAKE_CXX_STANDARD 23)

add_executable(Server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        src/Network/Server/ClientConnection.cpp
        include/Network/Server/ClientConnection.h
        include/Services/Services.h
        src/Services/ServerService.cpp
        include/Services/ServerService.h
        ../Common/include/Services/ServiceBase.h
        src/Services/Services.cpp
)
target_include_directories(Server PRIVATE include)
target_link_libraries(Server PRIVATE Common)