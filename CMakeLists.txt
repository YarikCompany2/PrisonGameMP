cmake_minimum_required(VERSION 4.0)
project(PrisonGameMP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor)

file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")

file(CREATE_LINK
    "${CMAKE_SOURCE_DIR}/assets"
    "${CMAKE_BINARY_DIR}/assets"
    SYMBOLIC
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib enet)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm
        ws2_32
        opengl32
        gdi32
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        )
    else()
        file(CREATE_LINK
            "${CMAKE_SOURCE_DIR}/assets"
            "${CMAKE_BINARY_DIR}/assets"
            SYMBOLIC
        )
    endif()
endif()