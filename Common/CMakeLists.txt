project(Common)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost REQUIRED COMPONENTS serialization)
set(LIBS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/include)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(FLATBUFFERS_GENERATED_DIR ${GENERATED_DIR}/fbs)

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
        main.cpp
        main.h
        src/Network/Packet.cpp
        include/Network/Packet.h
        src/Network/Packet.cpp
        include/Network/Packet.h
        src/Network/Packet.cpp
        include/Network/Packet.h
        src/Network/PacketHeader.cpp
        include/Network/PacketHeader.h
)

target_include_directories(Common PUBLIC ${LIBS_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${GENERATED_DIR})
target_link_libraries(Common PUBLIC Boost::serialization)