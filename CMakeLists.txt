cmake_minimum_required(VERSION 3.16)

project(ProceduralMazeExplorer
    VERSION 1.0.0
    DESCRIPTION "First-person procedural maze explorer (OpenGL 3.3 core + GLFW)"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(maze
    src/Glad.cpp
    src/Application.cpp
    src/DebugCapture.cpp
    src/Maze.cpp
    src/Player.cpp
    src/Renderer.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/Window.cpp
    src/main.cpp
)

target_include_directories(maze PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

# ---------------------------------------------------------------------------
# Dependencies: OpenGL + GLFW (downloaded at configure time via FetchContent)
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

include(FetchContent)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# CMake 4 removed compatibility with cmake_minimum_required(< 3.5).
# GLFW 3.4's scripts predate that, so opt back in when building with CMake >= 4.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
endif()

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

target_link_libraries(maze PRIVATE glfw OpenGL::GL)

# Bake the source-tree shader directory as a fallback lookup path and copy the
# shaders next to the executable so the build tree is self-contained.
target_compile_definitions(maze PRIVATE
    MAZE_SOURCE_SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_custom_command(TARGET maze POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:maze>/shaders
    COMMENT "Copying shaders next to executable")

# ---------------------------------------------------------------------------
# Windows portability
# ---------------------------------------------------------------------------
if(WIN32)
    # glfw3.h includes <windows.h> on Windows, whose min/max macros clash with
    # std::min/std::max. Define these before any translation unit sees it.
    target_compile_definitions(maze PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ---------------------------------------------------------------------------
# Warnings
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(maze PRIVATE /W4 /permissive-)
else()
    target_compile_options(maze PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()
