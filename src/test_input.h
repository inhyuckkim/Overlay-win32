#pragma once
#include <windows.h>

class TestInput {
public:
    void open(HINSTANCE hInstance);
    void close();
    bool isOpen() const { return hwnd_ != nullptr; }

private:
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    void sendText();

    HWND hwnd_ = nullptr;
    HWND editHwnd_ = nullptr;

    static constexpr int kWidth = 420;
    static constexpr int kHeight = 120;
    static constexpr int kEditId = 2001;
};
