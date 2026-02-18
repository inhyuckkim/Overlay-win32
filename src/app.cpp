#include "app.h"
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
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    overlayWindow_ = std::make_unique<OverlayWindow>();
    if (!overlayWindow_->create(hInstance, screenW, screenH))
        return false;

    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->init(overlayWindow_->hwnd(),
                         overlayWindow_->width(),
                         overlayWindow_->height()))
        return false;

    subtitleManager_ = std::make_unique<SubtitleManager>();

    messageHandler_ = std::make_unique<MessageHandler>(subtitleManager_.get());

    wsClient_ = std::make_unique<WsClient>();
    wsClient_->setUrl("ws://127.0.0.1:3000/overlay");
    wsClient_->setTargetHwnd(overlayWindow_->hwnd());
    wsClient_->start();

    menuButton_ = std::make_unique<MenuButton>();
    menuButton_->create(hInstance, screenW, screenH);

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
