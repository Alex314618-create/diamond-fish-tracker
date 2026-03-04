cmake_minimum_required(VERSION 3.20)
project(DiamondFishTracker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC UTF-8 support
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---- SDL2 (via vcpkg) ----
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

# ---- OpenCV (optional) ----
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found - camera support enabled")
else()
    message(STATUS "OpenCV not found - camera support disabled (OK)")
endif()

# ---- Dear ImGui (compiled from source in third_party/) ----
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    SDL2::SDL2
)

# MSVC: suppress ImGui's internal warnings (not our code)
if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
endif()

# ---- Main executable ----
add_executable(DiamondFishTracker
    src/main.cpp
)

target_include_directories(DiamondFishTracker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(DiamondFishTracker PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_ttf::SDL2_ttf
    imgui
)

if(OpenCV_FOUND)
    target_link_libraries(DiamondFishTracker PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(DiamondFishTracker PRIVATE HAVE_OPENCV)
endif()

# Windows: copy DLLs next to the .exe
if(WIN32)
    add_custom_command(TARGET DiamondFishTracker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:DiamondFishTracker>
            $<TARGET_FILE_DIR:DiamondFishTracker>
        COMMAND_EXPAND_LISTS
    )
endif()
