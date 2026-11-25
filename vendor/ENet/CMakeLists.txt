include(FetchContent)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(enet)

target_include_directories(enet PUBLIC "${enet_SOURCE_DIR}/include")

if(WIN32)
    target_link_libraries(enet PUBLIC ws2_32 winmm)
endif()