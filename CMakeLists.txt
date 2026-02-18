cmake_minimum_required(VERSION 3.20)
project(LixorOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime for single-exe distribution
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Fetch dependencies
include(FetchContent)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.5
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# Populate ixwebsocket and add as subdirectory (USE_ZLIB OFF from cache avoids ZLIB dependency)
FetchContent_Populate(ixwebsocket)
add_subdirectory(${ixwebsocket_SOURCE_DIR} ${ixwebsocket_BINARY_DIR} EXCLUDE_FROM_ALL)

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/overlay_window.cpp
    src/renderer.cpp
    src/subtitle_manager.cpp
    src/ws_client.cpp
    src/message_handler.cpp
    src/menu_button.cpp
    src/test_input.cpp
)

add_executable(LixorOverlay WIN32 ${SOURCES} resources/app.rc)

target_include_directories(LixorOverlay PRIVATE src)
target_link_libraries(LixorOverlay PRIVATE
    ixwebsocket
    nlohmann_json::nlohmann_json
    d2d1
    dwrite
    windowscodecs
    ws2_32
    shlwapi
)

target_compile_definitions(LixorOverlay PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
