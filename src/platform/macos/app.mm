#include "app.h"
#import <Cocoa/Cocoa.h>
#include <dispatch/dispatch.h>

#include "app_callbacks.h"
#include "message_handler.h"
#include "overlay_window.h"
#include "renderer.h"
#include "subtitle_manager.h"
#include "ws_client.h"
#include "menu_button.h"

App& App::instance() {
    static App app;
    return app;
}

bool App::init() {
    NSRect vf = [[NSScreen mainScreen] visibleFrame];
    int    screenW = static_cast<int>(NSWidth(vf));
    static constexpr int kOverlayBottomPadding = 8;
    int overlayBottomY = static_cast<int>(NSMinY(vf) + static_cast<CGFloat>(kOverlayBottomPadding));

    overlayWindow_ = std::make_unique<OverlayWindow>();
    if (!overlayWindow_->create(screenW, overlayBottomY))
        return false;

    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->init())
        return false;

    renderer_->resize(overlayWindow_->width(), overlayWindow_->height());

    subtitleManager_ = std::make_unique<SubtitleManager>();

    messageHandler_ = std::make_unique<MessageHandler>(
        subtitleManager_.get(),
        AppCallbacks{
            [this](int level) { setFontSizeLevel(level); },
            [this]() { requestRedraw(); },
        });

    wsClient_ = std::make_unique<WsClient>();
    wsClient_->setUrl("ws://127.0.0.1:3000/overlay");
    wsClient_->setMessageDeliverer([this](std::string payload) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (messageHandler_) messageHandler_->dispatch(payload);
            requestRedraw();
        });
    });
    wsClient_->start();

    menuButton_ = std::make_unique<MenuButton>();
    if (!menuButton_->create())
        return false;

    [NSTimer scheduledTimerWithTimeInterval:SubtitleManager::kTimerIntervalMs / 1000.0
                                       repeats:YES
                                         block:^(NSTimer* timer) {
                                           (void)timer;
                                           App::instance().subtitleTimerFired();
                                         }];

    overlayWindow_->show();
    requestRedraw();
    return true;
}

void App::shutdown() {
    if (wsClient_) wsClient_->stop();
    if (renderer_) renderer_->release();
    if (menuButton_) menuButton_->destroy();
    if (overlayWindow_) overlayWindow_->destroy();
}

void App::subtitleTimerFired() {
    if (subtitleManager_) {
        subtitleManager_->tick();
        requestRedraw();
    }
    if (overlayWindow_) overlayWindow_->reassertTopmost();
}

void App::requestRedraw() {
    dispatch_async(dispatch_get_main_queue(), ^{
        App::instance().performRedraw();
    });
}

void App::performRedraw() {
    auto* mgr = subtitleManager_.get();
    auto* ren = renderer_.get();
    auto* win = overlayWindow_.get();
    if (!mgr || !ren || !win) return;

    auto blocks = mgr->getVisibleBlocks();
    int requiredHeight = ren->measureHeight(blocks);
    win->setHeight(requiredHeight);
    ren->resize(win->width(), win->height());
    (void)blocks;
    win->invalidate();
}

void App::setFontSizeLevel(int level) {
    if (renderer_)
        renderer_->setFontSizeLevel(level);
    requestRedraw();
}
