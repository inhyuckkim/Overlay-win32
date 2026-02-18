#include "menu_button.h"
#include "app.h"
#include "subtitle_manager.h"
#include "overlay_window.h"
#include "test_input.h"

static constexpr wchar_t kMenuBtnClass[] = L"LixorMenuBtn";

enum MenuCmd : UINT {
    CMD_TOGGLE_SUBTITLES = 1001,
    CMD_TEST_SUBTITLE,
    CMD_RESET,
    CMD_EXIT,
};

bool MenuButton::create(HINSTANCE hInstance, int screenW, int screenH) {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kMenuBtnClass;
    wc.hCursor       = LoadCursor(nullptr, IDC_HAND);
    RegisterClassExW(&wc);

    int x = screenW - kSize - kMargin;
    int y = screenH - kSize - kMargin;

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        kMenuBtnClass, L"",
        WS_POPUP,
        x, y, kSize, kSize,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd_) return false;

    SetLayeredWindowAttributes(hwnd_, 0, 200, LWA_ALPHA);
    return true;
}

void MenuButton::destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void MenuButton::show() {
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
}

void MenuButton::hide() {
    ShowWindow(hwnd_, SW_HIDE);
}

LRESULT CALLBACK MenuButton::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* btn = App::instance().menuButton();

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (btn) btn->paintButton(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONUP:
        if (btn) btn->showPopupMenu();
        return 0;
    case WM_MOUSEMOVE: {
        if (btn && !btn->hovered_) {
            btn->hovered_ = true;
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        if (btn) {
            btn->hovered_ = false;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_COMMAND: {
        auto* mgr = App::instance().subtitleManager();
        switch (LOWORD(wp)) {
        case CMD_TOGGLE_SUBTITLES:
            if (mgr) {
                bool cur = mgr->isVisible();
                mgr->setVisible(!cur);
                App::instance().requestRedraw();
            }
            break;
        case CMD_TEST_SUBTITLE: {
            auto* ti = App::instance().testInput();
            if (ti) ti->open(App::instance().hInstance());
            break;
        }
        case CMD_RESET:
            if (mgr) {
                mgr->reset();
                App::instance().requestRedraw();
            }
            break;
        case CMD_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MenuButton::showPopupMenu() {
    auto* mgr = App::instance().subtitleManager();
    HMENU hMenu = CreatePopupMenu();

    bool visible = mgr ? mgr->isVisible() : true;
    AppendMenuW(hMenu, MF_STRING, CMD_TOGGLE_SUBTITLES,
                visible ? L"Hide Subtitles" : L"Show Subtitles");
    AppendMenuW(hMenu, MF_STRING, CMD_TEST_SUBTITLE, L"Test Subtitle...");
    AppendMenuW(hMenu, MF_STRING, CMD_RESET, L"Reset");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, CMD_EXIT, L"Exit Overlay");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(hMenu);
}

void MenuButton::paintButton(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);

    HBRUSH bg = CreateSolidBrush(hovered_ ? RGB(80, 80, 80) : RGB(50, 50, 50));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    // Draw "L" letter as icon
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT font = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(hdc, font);
    DrawTextW(hdc, L"L", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}
