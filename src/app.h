#pragma once
#include <windows.h>
#include <memory>

class OverlayWindow;
class Renderer;
class SubtitleManager;
class WsClient;
class MessageHandler;
class MenuButton;

class App {
public:
    static App& instance();

    bool init(HINSTANCE hInstance);
    int  run();
    void shutdown();

    HINSTANCE       hInstance() const { return hInstance_; }
    OverlayWindow*  overlayWindow() const { return overlayWindow_.get(); }
    Renderer*       renderer() const { return renderer_.get(); }
    SubtitleManager* subtitleManager() const { return subtitleManager_.get(); }
    WsClient*       wsClient() const { return wsClient_.get(); }
    MessageHandler* messageHandler() const { return messageHandler_.get(); }
    MenuButton*     menuButton() const { return menuButton_.get(); }

    void requestRedraw();

private:
    App() = default;

    HINSTANCE hInstance_ = nullptr;
    std::unique_ptr<OverlayWindow>   overlayWindow_;
    std::unique_ptr<Renderer>        renderer_;
    std::unique_ptr<SubtitleManager> subtitleManager_;
    std::unique_ptr<WsClient>        wsClient_;
    std::unique_ptr<MessageHandler>  messageHandler_;
    std::unique_ptr<MenuButton>      menuButton_;
};
