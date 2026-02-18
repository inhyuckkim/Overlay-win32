#pragma once
#include <windows.h>

class MenuButton {
public:
    bool create(HINSTANCE hInstance, int screenW, int screenH);
    void destroy();
    HWND hwnd() const { return hwnd_; }

    void show();
    void hide();

private:
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    void showPopupMenu();
    void paintButton(HDC hdc);

    HWND hwnd_ = nullptr;
    bool hovered_ = false;

    static constexpr int kSize = 36;
    static constexpr int kMargin = 16;
};
