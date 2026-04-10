#pragma once
#include <string>

#include "app_callbacks.h"

class SubtitleManager;

class MessageHandler {
public:
    MessageHandler(SubtitleManager* mgr, AppCallbacks callbacks);

    void dispatch(const std::string& json);

private:
    SubtitleManager* mgr_;
    AppCallbacks     callbacks_;
};
