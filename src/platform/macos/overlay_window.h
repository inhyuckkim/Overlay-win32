#pragma once
#include <memory>

class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool create(int screenW, int overlayBottomY);
    void destroy();

    void show();
    void hide();
    void reassertTopmost();

    void setHeight(int height);

    int width() const;
    int height() const;

    void invalidate();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
