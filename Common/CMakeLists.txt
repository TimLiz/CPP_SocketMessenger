include(FetchContent)

project(Common)

set(LIBS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/include)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(FLATBUFFERS_GENERATED_DIR ${GENERATED_DIR}/fbs)

# Packages
find_package(Boost CONFIG REQUIRED core)
find_package(spdlog CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)

# FlatBuffer generation
file(
        GLOB_RECURSE fbsFiles
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/fbs
        *.fbs
)

foreach (fbs ${fbsFiles})
    get_filename_component(name ${fbs} NAME_WE)

    set(out ${FLATBUFFERS_GENERATED_DIR}/${name}_generated.h)

    add_custom_command(
            OUTPUT ${out}
            COMMAND flatbuffers::flatc -o ${FLATBUFFERS_GENERATED_DIR} --cpp ${CMAKE_CURRENT_SOURCE_DIR}/fbs/${fbs}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fbs/${fbs}
            COMMENT "Generating header for ${fbs}"
    )

    list(APPEND GENERATED_FBS_HEADERS ${out})
endforeach ()

add_library(Common STATIC
        ${GENERATED_FBS_HEADERS}
        src/Network/PacketView.cpp
        include/Network/PacketView.h
        src/Network/PacketView.cpp
        include/Network/PacketView.h
        src/Network/PacketView.cpp
        include/Network/PacketView.h
        src/Network/PacketHeader.cpp
        include/Network/PacketHeader.h
        src/Network/Socket.cpp
        include/Network/Socket.h
        src/Epoll.cpp
        include/Epoll.h
        include/Services/PacketsDispatchService/PacketsDispatchService.h
        src/Network/Peer.cpp
        include/Network/Peer.h
        src/Network/SecurePeer.cpp
        include/Network/SecurePeer.h
)

target_include_directories(Common PUBLIC ${LIBS_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${GENERATED_DIR} include)

# Linking libraries
target_link_libraries(Common PUBLIC Boost::core)
target_link_libraries(Common PUBLIC spdlog::spdlog)
target_link_libraries(Common PUBLIC flatbuffers::flatbuffers)

target_compile_definitions(Common PUBLIC
        $<$<CONFIG:Debug>:IS_DEBUG=1>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)