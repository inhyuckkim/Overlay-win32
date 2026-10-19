#pragma once
#include <windows.h>

class MenuButton {
public:
    bool create(HINSTANCE hInstance, int screenX, int screenW, int overlayBottomY);
    void destroy();
    HWND hwnd() const { return hwnd_; }

    void show();
    void hide();
    /** Follow the caption strip onto another monitor's work area. */
    void moveTo(int screenX, int screenW, int overlayBottomY);

private:
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    void showPopupMenu();
    void paintButton(HDC hdc);

    HWND hwnd_    = nullptr;
    bool hovered_ = false;

    static constexpr int kSize   = 36;
    static constexpr int kMargin = 16;
};
