#pragma once
#include <windows.h>
#include <functional>

constexpr UINT WM_OVERLAY_MSG = WM_APP + 1;
constexpr UINT WM_OVERLAY_REDRAW = WM_APP + 2;

class OverlayWindow {
public:
    bool create(HINSTANCE hInstance, int screenW, int screenH);
    void destroy();

    HWND hwnd() const { return hwnd_; }
    int  width() const { return width_; }
    int  height() const { return height_; }
    int  posY() const { return posY_; }

    void show();
    void hide();
    void reassertTopmost();

private:
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

    HWND hwnd_ = nullptr;
    int  width_  = 0;
    int  height_ = 0;
    int  posY_   = 0;
};
