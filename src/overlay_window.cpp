#include "overlay_window.h"
#include "app.h"
#include "renderer.h"
#include "subtitle_manager.h"
#include "message_handler.h"
#include <string>

static constexpr wchar_t kClassName[] = L"LixorOverlayWnd";
static constexpr int kOverlayHeight = 480;

bool OverlayWindow::create(HINSTANCE hInstance, int screenW, int screenH) {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    width_   = screenW;
    height_  = kOverlayHeight;
    screenH_ = screenH;
    posY_    = screenH - height_;

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kClassName, L"Lixor Overlay",
        WS_POPUP,
        0, posY_, width_, height_,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd_) return false;

    // No SetLayeredWindowAttributes - we use UpdateLayeredWindow for per-pixel alpha
    return true;
}

void OverlayWindow::destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void OverlayWindow::show() {
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    reassertTopmost();
}

void OverlayWindow::hide() {
    ShowWindow(hwnd_, SW_HIDE);
}

void OverlayWindow::reassertTopmost() {
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void OverlayWindow::setHeight(int h) {
    height_ = (h > 0 ? h : 0);
    posY_   = screenH_ - height_;
    if (hwnd_)
        SetWindowPos(hwnd_, nullptr, 0, posY_, width_, height_,
                     SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK OverlayWindow::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_OVERLAY_MSG: {
        // Heap-allocated string from WsClient background thread
        auto* payload = reinterpret_cast<std::string*>(lp);
        if (payload) {
            auto* handler = App::instance().messageHandler();
            if (handler) handler->dispatch(*payload);
            delete payload;
            App::instance().requestRedraw();
        }
        return 0;
    }
    case WM_OVERLAY_REDRAW: {
        auto* mgr = App::instance().subtitleManager();
        auto* ren = App::instance().renderer();
        auto* win = App::instance().overlayWindow();
        if (mgr && ren && win) {
            auto blocks = mgr->getVisibleBlocks();
            int requiredHeight = ren->measureHeight(blocks);
            win->setHeight(requiredHeight);
            ren->resize(win->width(), win->height());
            if (requiredHeight > 0)
                ren->render(blocks);
        }
        return 0;
    }
    case WM_TIMER: {
        if (wp == SubtitleManager::kTimerId) {
            auto* mgr = App::instance().subtitleManager();
            if (mgr) {
                mgr->tick();
                App::instance().requestRedraw();
            }
            // Periodically re-assert topmost in case a fullscreen app stole focus
            auto* win = App::instance().overlayWindow();
            if (win) win->reassertTopmost();
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
