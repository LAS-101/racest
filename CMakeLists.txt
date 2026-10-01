cmake_minimum_required(VERSION 3.16)
project(racest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
endif()

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(racest src/main.cpp src/game.cpp src/renderer.cpp)
target_link_libraries(racest PRIVATE raylib)
target_compile_options(racest PRIVATE -Wall -Wextra)

if(EMSCRIPTEN)
    set_target_properties(racest PROPERTIES
        OUTPUT_NAME index
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/public"
    )
    target_link_options(racest PRIVATE
        "-sUSE_GLFW=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web"
        "SHELL:--shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\""
        "SHELL:--preload-file \"${CMAKE_SOURCE_DIR}/Words.txt@/Words.txt\""
    )
    set_property(TARGET racest APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/web/shell.html" "${CMAKE_SOURCE_DIR}/Words.txt")
else()
    configure_file(Words.txt Words.txt COPYONLY)
endif()
