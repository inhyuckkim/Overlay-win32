#pragma once
#include <string>

class SubtitleManager;

class MessageHandler {
public:
    explicit MessageHandler(SubtitleManager* mgr);
    void dispatch(const std::string& json);

private:
    SubtitleManager* mgr_;
};
