#include "app.h"
#include "app_callbacks.h"
#include "overlay_window.h"
#include "renderer.h"
#include "subtitle_manager.h"
#include "ws_client.h"
#include "message_handler.h"
#include "menu_button.h"
#include "test_input.h"
#include <string>
#include <fstream>
#include <windows.h>

// ---------------------------------------------------------------------------
// Config: read ws_url from lixorhost.conf next to the .exe.
// Format (plain text):
//   ws_url = wss://lixorhost.datant.ca/overlay
// Lines starting with '#' are comments. Absent or malformed → localhost default.
// ---------------------------------------------------------------------------
static std::string readWsUrlFromConfig() {
    const std::string kDefault = "ws://127.0.0.1:3000/overlay";

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeDir(exePath);
    auto sep = exeDir.rfind(L'\\');
    if (sep != std::wstring::npos) exeDir = exeDir.substr(0, sep);

    std::ifstream file(exeDir + L"\\lixorhost.conf");
    if (!file.is_open()) return kDefault;

    std::string line;
    while (std::getline(file, line)) {
        // Trim leading whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;
        if (line.find("ws_url") == 0) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string val = line.substr(eq + 1);
                auto vs = val.find_first_not_of(" \t");
                auto ve = val.find_last_not_of(" \t\r\n");
                if (vs != std::string::npos)
                    return val.substr(vs, ve - vs + 1);
            }
        }
    }
    return kDefault;
}

App& App::instance() {
    static App app;
    return app;
}

bool App::init(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    /** Pixels above work-area bottom; keep small so captions sit just above the taskbar. */
    static constexpr int kOverlayBottomPadding = 2;
    int overlayBottomY = workArea.bottom - kOverlayBottomPadding;

    overlayWindow_ = std::make_unique<OverlayWindow>();
    if (!overlayWindow_->create(hInstance, screenW, overlayBottomY))
        return false;

    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->init(overlayWindow_->hwnd(),
                         overlayWindow_->width(),
                         overlayWindow_->height()))
        return false;

    subtitleManager_ = std::make_unique<SubtitleManager>();

    messageHandler_ = std::make_unique<MessageHandler>(
        subtitleManager_.get(),
        AppCallbacks{
            [this](int level) { setFontSizeLevel(level); },
            [this]() { requestRedraw(); },
        });

    wsClient_ = std::make_unique<WsClient>();
    wsClient_->setUrl(readWsUrlFromConfig());
    HWND hwnd = overlayWindow_->hwnd();
    wsClient_->setMessageDeliverer([hwnd](std::string payload) {
        auto* copy = new std::string(std::move(payload));
        PostMessage(hwnd, WM_OVERLAY_MSG, 0, reinterpret_cast<LPARAM>(copy));
    });
    wsClient_->start();

    menuButton_ = std::make_unique<MenuButton>();
    menuButton_->create(hInstance, screenW, overlayBottomY);

    testInput_ = std::make_unique<TestInput>();

    SetTimer(overlayWindow_->hwnd(),
             SubtitleManager::kTimerId,
             SubtitleManager::kTimerIntervalMs,
             nullptr);

    overlayWindow_->show();
    menuButton_->show();
    requestRedraw();
    return true;
}

int App::run() {
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void App::shutdown() {
    if (testInput_) testInput_->close();
    wsClient_->stop();
    renderer_->release();
    menuButton_->destroy();
    overlayWindow_->destroy();
}

void App::requestRedraw() {
    if (overlayWindow_ && overlayWindow_->hwnd()) {
        PostMessage(overlayWindow_->hwnd(), WM_OVERLAY_REDRAW, 0, 0);
    }
}

void App::setFontSizeLevel(int level) {
    if (renderer_)
        renderer_->setFontSizeLevel(level);
    requestRedraw();
}
