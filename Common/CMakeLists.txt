include(FetchContent)

project(Common)

set(CMAKE_CXX_STANDARD 23)
set(SPDLOG_VERSION "v1.16.0")



find_package(Boost REQUIRED COMPONENTS serialization)
set(LIBS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/include)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(FLATBUFFERS_GENERATED_DIR ${GENERATED_DIR}/fbs)

# Installing SPDLOG
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG ${SPDLOG_VERSION}
)
FetchContent_MakeAvailable(spdlog)

# FlatBuffer generation
file(
        GLOB_RECURSE fbsFiles
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/fbs
        *.fbs
)

foreach(fbs ${fbsFiles})
    get_filename_component(name ${fbs} NAME_WE)

    set(out ${FLATBUFFERS_GENERATED_DIR}/${name}_generated.h)

    add_custom_command(
            OUTPUT ${out}
            COMMAND flatc -o ${FLATBUFFERS_GENERATED_DIR} --cpp ${CMAKE_CURRENT_SOURCE_DIR}/fbs/${fbs}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fbs/${fbs}
            COMMENT "Generating header for ${fbs}"
    )

    list(APPEND GENERATED_FBS_HEADERS ${out})
endforeach()

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
)

target_include_directories(Common PUBLIC ${LIBS_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${GENERATED_DIR} include)
target_link_libraries(Common PUBLIC Boost::serialization spdlog::spdlog)


target_compile_definitions(Common PUBLIC
        $<$<CONFIG:Debug>:IS_DEBUG=1>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)