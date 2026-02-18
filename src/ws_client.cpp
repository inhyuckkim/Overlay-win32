#include "ws_client.h"
#include "overlay_window.h"
#include <windows.h>

void WsClient::setUrl(const std::string& url) {
    ws_.setUrl(url);
}

void WsClient::setTargetHwnd(HWND hwnd) {
    targetHwnd_ = hwnd;
}

void WsClient::setOnMessage(MessageCallback cb) {
    onMessage_ = std::move(cb);
}

void WsClient::start() {
    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::string payload = msg->str;
            if (targetHwnd_) {
                auto* copy = new std::string(std::move(payload));
                PostMessage(targetHwnd_, WM_OVERLAY_MSG,
                            0, reinterpret_cast<LPARAM>(copy));
            }
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            OutputDebugStringA("[WsClient] Connected\n");
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            OutputDebugStringA("[WsClient] Disconnected\n");
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            OutputDebugStringA("[WsClient] Error: ");
            OutputDebugStringA(msg->errorInfo.reason.c_str());
            OutputDebugStringA("\n");
        }
    });

    ws_.enableAutomaticReconnection();
    ws_.setMinWaitBetweenReconnectionRetries(1000);
    ws_.setMaxWaitBetweenReconnectionRetries(30000);
    ws_.start();
}

void WsClient::stop() {
    ws_.stop();
}

bool WsClient::isConnected() const {
    return ws_.getReadyState() == ix::ReadyState::Open;
}
