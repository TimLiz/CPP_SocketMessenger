project(Client)

set(CMAKE_CXX_STANDARD 23)

add_executable(Client ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(Client PRIVATE Common)