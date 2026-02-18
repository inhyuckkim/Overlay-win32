#include "test_input.h"
#include "app.h"
#include "subtitle_manager.h"

static constexpr wchar_t kTestInputClass[] = L"LixorTestInput";
static constexpr int kPad = 10;
static constexpr int kEditHeight = 28;
static constexpr int kBtnWidth = 70;

void TestInput::open(HINSTANCE hInstance) {
    if (hwnd_) {
        SetForegroundWindow(hwnd_);
        return;
    }

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kTestInputClass;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - kWidth) / 2;
    int y = (screenH - kHeight) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kTestInputClass, L"Test Subtitle",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, kWidth, kHeight,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd_) return;

    RECT clientRc;
    GetClientRect(hwnd_, &clientRc);
    int cw = clientRc.right - clientRc.left;
    int ch = clientRc.bottom - clientRc.top;

    CreateWindowExW(0, L"STATIC", L"Type text and press Enter:",
                    WS_CHILD | WS_VISIBLE,
                    kPad, kPad, cw - kPad * 2, 18,
                    hwnd_, nullptr, hInstance, nullptr);

    int editW = cw - kPad * 3 - kBtnWidth;
    int editY = kPad + 22;

    editHwnd_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        kPad, editY, editW, kEditHeight,
        hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)),
        hInstance, nullptr);

    CreateWindowExW(0, L"BUTTON", L"Send",
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    kPad + editW + kPad, editY, kBtnWidth, kEditHeight,
                    hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDOK)),
                    hInstance, nullptr);

    HFONT font = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    SendMessage(editHwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
    SetFocus(editHwnd_);
}

void TestInput::close() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        editHwnd_ = nullptr;
    }
}

void TestInput::sendText() {
    if (!editHwnd_) return;

    int len = GetWindowTextLengthW(editHwnd_);
    if (len <= 0) return;

    std::wstring text(len, L'\0');
    GetWindowTextW(editHwnd_, text.data(), len + 1);

    auto* mgr = App::instance().subtitleManager();
    if (mgr) {
        mgr->updateSubtitle(L"test", text, true);
        App::instance().requestRedraw();
    }

    SetWindowTextW(editHwnd_, L"");
    SetFocus(editHwnd_);
}

LRESULT CALLBACK TestInput::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* ti = App::instance().testInput();

    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && ti) {
            ti->sendText();
            return 0;
        }
        break;
    case WM_CLOSE:
        if (ti) ti->close();
        return 0;
    case WM_DESTROY:
        if (ti) {
            ti->hwnd_ = nullptr;
            ti->editHwnd_ = nullptr;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
