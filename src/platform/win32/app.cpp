#include "app.h"
#include "app_callbacks.h"
#include "overlay_window.h"
#include "renderer.h"
#include "subtitle_manager.h"
#include "ws_client.h"
#include "message_handler.h"
#include "menu_button.h"
#include "test_input.h"
#include <algorithm>
#include <string>
#include <fstream>
#include <windows.h>

// ---------------------------------------------------------------------------
// Config: read ws_url from lixorhost.conf next to the .exe.
// Format (plain text):
//   ws_url = wss://lixorhost.datant.ca/overlay
// Lines starting with '#' are comments. Absent or malformed -> localhost default.
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

// ---------------------------------------------------------------------------
// Monitor enumeration
// ---------------------------------------------------------------------------

static BOOL CALLBACK collectMonitor(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMon, &mi)) {
        MonitorRects m;
        m.full    = mi.rcMonitor;
        m.work    = mi.rcWork;
        m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        reinterpret_cast<std::vector<MonitorRects>*>(lParam)->push_back(m);
    }
    return TRUE;
}

std::vector<MonitorRects> enumerateMonitorsSorted() {
    std::vector<MonitorRects> monitors;
    EnumDisplayMonitors(nullptr, nullptr, collectMonitor,
                        reinterpret_cast<LPARAM>(&monitors));

    // Sort by (left, top) ascending. The Lixor Host server applies the identical
    // sort to its display list, so index N means the same screen on both sides.
    std::sort(monitors.begin(), monitors.end(),
              [](const MonitorRects& a, const MonitorRects& b) {
                  if (a.full.left != b.full.left) return a.full.left < b.full.left;
                  return a.full.top < b.full.top;
              });

    if (monitors.empty()) {
        // EnumDisplayMonitors should never come back empty, but never hand the
        // caller an empty list - fall back to the primary screen metrics.
        MonitorRects m;
        m.full = RECT{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        m.work = m.full;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &m.work, 0);
        m.primary = true;
        monitors.push_back(m);
    }
    return monitors;
}

/** Pixels above the work-area bottom; keep small so captions sit just above the taskbar. */
static constexpr int kOverlayBottomPadding = 2;

App& App::instance() {
    static App app;
    return app;
}

bool App::init(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // Start on the first monitor in (left, top) order; the host pushes the
    // user's saved "set_display" right after connecting.
    auto monitors  = enumerateMonitorsSorted();
    displayIndex_  = 0;
    const RECT& work = monitors[0].work;
    int screenX        = work.left;
    int screenW        = work.right - work.left;
    int overlayBottomY = work.bottom - kOverlayBottomPadding;

    overlayWindow_ = std::make_unique<OverlayWindow>();
    if (!overlayWindow_->create(hInstance, screenX, screenW, overlayBottomY))
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
            [this](const OverlayColors& c) { setColors(c); },
            [this](int index) { setDisplayIndex(index); },
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
    menuButton_->create(hInstance, screenX, screenW, overlayBottomY);

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

void App::setColors(const OverlayColors& colors) {
    if (renderer_)
        renderer_->setColors(colors);
    requestRedraw();
}

void App::setDisplayIndex(int index) {
    displayIndex_ = index < 0 ? 0 : index;
    applyDisplayPlacement();
}

void App::handleDisplayChange() {
    // A monitor was unplugged / resolution changed: re-place on the current
    // index, which applyDisplayPlacement() clamps back to 0 if it is now gone.
    applyDisplayPlacement();
}

void App::applyDisplayPlacement() {
    auto monitors = enumerateMonitorsSorted();
    if (displayIndex_ < 0 || displayIndex_ >= static_cast<int>(monitors.size())) {
        // Selected monitor is gone (or was never there) - fall back to the first one.
        displayIndex_ = 0;
    }

    const RECT& work = monitors[displayIndex_].work;
    int screenX        = work.left;
    int screenW        = work.right - work.left;
    int overlayBottomY = work.bottom - kOverlayBottomPadding;

    if (overlayWindow_)
        overlayWindow_->setMonitorBounds(screenX, screenW, overlayBottomY);
    if (renderer_ && overlayWindow_)
        renderer_->resize(overlayWindow_->width(), overlayWindow_->height());
    if (menuButton_)
        menuButton_->moveTo(screenX, screenW, overlayBottomY);

    requestRedraw();
}
