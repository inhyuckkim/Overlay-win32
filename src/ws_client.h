#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <ixwebsocket/IXWebSocket.h>

class WsClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;

    void setUrl(const std::string& url);
    void setTargetHwnd(HWND hwnd);
    void setOnMessage(MessageCallback cb);
    void start();
    void stop();
    bool isConnected() const;

private:
    ix::WebSocket ws_;
    HWND          targetHwnd_ = nullptr;
    MessageCallback onMessage_;
};
