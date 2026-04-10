cmake_minimum_required(VERSION 3.20)

if(APPLE)
    project(LixorOverlay LANGUAGES CXX OBJCXX)
else()
    project(LixorOverlay LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OBJCXX_STANDARD 17)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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

FetchContent_Populate(ixwebsocket)
add_subdirectory(${ixwebsocket_SOURCE_DIR} ${ixwebsocket_BINARY_DIR} EXCLUDE_FROM_ALL)

set(COMMON_SOURCES
    src/common/subtitle_manager.cpp
    src/common/message_handler.cpp
)

if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/win32/main.cpp
        src/platform/win32/app.cpp
        src/platform/win32/overlay_window.cpp
        src/platform/win32/renderer.cpp
        src/platform/win32/ws_client.cpp
        src/platform/win32/menu_button.cpp
        src/platform/win32/test_input.cpp
    )
    add_executable(LixorOverlay WIN32 ${COMMON_SOURCES} ${PLATFORM_SOURCES} resources/win32/app.rc)

    target_include_directories(LixorOverlay PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/platform/win32
    )

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
elseif(APPLE)
    set(PLATFORM_SOURCES
        src/platform/macos/main.mm
        src/platform/macos/app.mm
        src/platform/macos/overlay_window.mm
        src/platform/macos/renderer.mm
        src/platform/macos/ws_client.cpp
        src/platform/macos/menu_button.mm
    )
    add_executable(LixorOverlay MACOSX_BUNDLE
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
    )

    set_target_properties(LixorOverlay PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER com.lixor.overlay
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )

    target_compile_options(LixorOverlay PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")

    target_include_directories(LixorOverlay PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/platform/macos
    )

    target_link_libraries(LixorOverlay PRIVATE
        ixwebsocket
        nlohmann_json::nlohmann_json
        "-framework AppKit"
        "-framework CoreText"
        "-framework CoreGraphics"
    )
else()
    message(FATAL_ERROR "Unsupported platform (only Windows and macOS are configured).")
endif()
