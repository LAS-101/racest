cmake_minimum_required(VERSION 3.16)
project(racest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
endif()

if(NOT EMSCRIPTEN)
    find_package(raylib 5.5 QUIET)
endif()

if(NOT TARGET raylib)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(racest
    src/game/main.cpp
    src/game/game.cpp
    src/game/renderer.cpp
    src/game/game_bridge.cpp
)
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
        "SHELL:--preload-file \"${CMAKE_SOURCE_DIR}/assets/words.txt@/assets/words.txt\""
    )
    set_property(TARGET racest APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/web/shell.html" "${CMAKE_SOURCE_DIR}/assets/words.txt")
else()
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/assets")
    configure_file(assets/words.txt assets/words.txt COPYONLY)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(game_tests tests/game/game_tests.cpp src/game/game.cpp)
        target_include_directories(game_tests PRIVATE src/game)
        target_link_libraries(game_tests PRIVATE raylib)
        target_compile_options(game_tests PRIVATE -Wall -Wextra)
        add_test(NAME game_tests COMMAND game_tests)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(NAME python_agent_tests COMMAND "${Python3_EXECUTABLE}"
                -m unittest discover -s "${CMAKE_SOURCE_DIR}/tests/agent" -v)
            set_tests_properties(python_agent_tests PROPERTIES
                ENVIRONMENT "RACEST_TEST_BINARY=$<TARGET_FILE:racest>")
        endif()
    endif()
endif()
