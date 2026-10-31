cmake_minimum_required(VERSION 3.20)
project(MinecraftClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(MinecraftClone 
    src/main.cpp
    src/block.cpp
    src/chunk.cpp
    src/world.cpp
    src/player.cpp
    src/utils.cpp
    src/ui.cpp
    src/texture_atlas.cpp
    src/thread_pool.cpp
    src/inventory.cpp
)

target_include_directories(MinecraftClone PRIVATE src)
target_link_libraries(MinecraftClone PRIVATE raylib)