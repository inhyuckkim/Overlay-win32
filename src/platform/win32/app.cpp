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
    wsClient_->setUrl("ws://127.0.0.1:3000/overlay");
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
