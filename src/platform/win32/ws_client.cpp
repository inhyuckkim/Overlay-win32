#include "ws_client.h"

void WsClient::setUrl(const std::string& url) {
    ws_.setUrl(url);
}

void WsClient::setMessageDeliverer(MessageDeliverer fn) {
    deliver_ = std::move(fn);
}

void WsClient::start() {
    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::string payload = msg->str;
            if (deliver_) deliver_(std::move(payload));
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
