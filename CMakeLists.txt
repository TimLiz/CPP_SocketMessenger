cmake_minimum_required(VERSION 4.1)
project(myMessenger)

set(CMAKE_CXX_STANDARD 23)

option(ENABLE_SECURE_TRANSPORT "Enable secure transport" ON)

add_subdirectory(Common)
add_subdirectory(Server)
add_subdirectory(Client)

target_link_libraries(Common PUBLIC -fsanitize=address)