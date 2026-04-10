#pragma once
#include <functional>
#include <ixwebsocket/IXWebSocket.h>
#include <string>

class WsClient {
public:
    using MessageDeliverer = std::function<void(std::string)>;

    void setUrl(const std::string& url);
    void setMessageDeliverer(MessageDeliverer fn);
    void start();
    void stop();
    bool isConnected() const;

private:
    ix::WebSocket    ws_;
    MessageDeliverer deliver_;
};
