cmake_minimum_required(VERSION 3.20)
project(GeomLine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)

# Download GLFW.
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Download Dear ImGui.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        5c7c293a66d7a659d4e6a1abdf18291ce004c07e # Pinned Dear ImGui revision.
)
FetchContent_MakeAvailable(imgui)

# Build ImGui with the GLFW and OpenGL backends.
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# Application executable.
add_executable(app
    Source/main.cpp
    Source/geometry.cpp
)

target_include_directories(app PRIVATE Source)
target_link_libraries(app PRIVATE
    OpenMP::OpenMP_CXX
    imgui_lib
)

include(CTest)
if(BUILD_TESTING)
    add_executable(solver_tests Tests/solvers.cpp Source/geometry.cpp)
    target_include_directories(solver_tests PRIVATE Source)
    target_link_libraries(solver_tests PRIVATE OpenMP::OpenMP_CXX)
    add_test(NAME solver_consistency COMMAND solver_tests)
endif()
