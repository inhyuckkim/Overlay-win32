#pragma once
#include <string>

class SubtitleManager;
class App;

class MessageHandler {
public:
    MessageHandler(SubtitleManager* mgr, App* app);
    void dispatch(const std::string& json);

private:
    SubtitleManager* mgr_;
    App* app_;
};
