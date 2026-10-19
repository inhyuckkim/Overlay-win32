#pragma once
#include <functional>

/** Subtitle colors pushed from the host. Channels are 0.0-1.0; bgOpacity is the
 *  background's own alpha (multiplied by each block's fade opacity at draw time). */
struct OverlayColors {
    float textR = 1.0f, textG = 1.0f, textB = 1.0f;
    float bgR   = 0.0f, bgG   = 0.0f, bgB   = 0.0f;
    float bgOpacity = 0.70f;
};

// New members go at the END: app.mm initializes only the first two positionally,
// so anything appended stays an empty std::function there. Always call through a
// `if (callbacks_.x)` guard.
struct AppCallbacks {
    std::function<void(int)> setFontSizeLevel;
    std::function<void()>      requestRedraw;
    std::function<void(const OverlayColors&)> setColors;
    std::function<void(int)>                  setDisplayIndex;
};
