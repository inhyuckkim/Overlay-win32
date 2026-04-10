#pragma once
#include <functional>

struct AppCallbacks {
    std::function<void(int)> setFontSizeLevel;
    std::function<void()>      requestRedraw;
};
