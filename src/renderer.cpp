#include "renderer.h"
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

bool Renderer::init(HWND hwnd, int width, int height) {
    hwnd_   = hwnd;
    width_  = width;
    height_ = height;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                             __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(dwFactory_.GetAddressOf()));
    if (FAILED(hr)) return false;

    hr = dwFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        kFontSize, L"", textFormat_.GetAddressOf());
    if (FAILED(hr)) return false;
    textFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    textFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

    hr = dwFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        kLabelSize, L"", labelFormat_.GetAddressOf());
    if (FAILED(hr)) return false;
    labelFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    createDeviceResources();
    return true;
}

void Renderer::createDeviceResources() {
    if (rt_) return;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0);

    d2dFactory_->CreateDCRenderTarget(&props, rt_.GetAddressOf());
    if (!rt_) return;

    rt_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), textBrush_.GetAddressOf());
    rt_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.70f), bgBrush_.GetAddressOf());
    rt_->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f), labelBrush_.GetAddressOf());
}

void Renderer::discardDeviceResources() {
    textBrush_.Reset();
    bgBrush_.Reset();
    labelBrush_.Reset();
    rt_.Reset();
}

void Renderer::release() {
    discardDeviceResources();
    textFormat_.Reset();
    labelFormat_.Reset();
    dwFactory_.Reset();
    d2dFactory_.Reset();
}

void Renderer::resize(int width, int height) {
    width_  = width;
    height_ = height;
}

void Renderer::render(const std::vector<SubtitleBlock>& blocks) {
    createDeviceResources();
    if (!rt_) return;

    HDC screenDC = GetDC(nullptr);
    HDC memDC    = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width_;
    bmi.bmiHeader.biHeight      = -height_;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }
    HGDIOBJ oldBmp = SelectObject(memDC, hBmp);

    RECT rc{0, 0, width_, height_};
    HRESULT hr = rt_->BindDC(memDC, &rc);
    if (FAILED(hr)) {
        SelectObject(memDC, oldBmp);
        DeleteObject(hBmp);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    rt_->BeginDraw();
    rt_->Clear(D2D1::ColorF(0, 0, 0, 0));

    if (!blocks.empty()) {
        float maxTextWidth = width_ * 0.85f;
        float curY = static_cast<float>(height_);

        for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
            const auto& b = blocks[i];
            if (b.text.empty()) continue;

            ComPtr<IDWriteTextLayout> measureLayout;
            dwFactory_->CreateTextLayout(
                b.text.c_str(), static_cast<UINT32>(b.text.size()),
                textFormat_.Get(), maxTextWidth, 1000.0f,
                measureLayout.GetAddressOf());
            if (!measureLayout) continue;

            DWRITE_TEXT_METRICS metrics{};
            measureLayout->GetMetrics(&metrics);
            float lineH = kFontSize * 1.3f;
            float textH = (std::min)(metrics.height, lineH * 2);

            float labelH = kLabelSize + 4.0f;
            float blockH = kBlockPadV + labelH + textH + kBlockPadV;
            float blockW = (std::min)(metrics.widthIncludingTrailingWhitespace + kBlockPadH * 2, static_cast<float>(width_));
            blockW = (std::max)(blockW, 200.0f);
            float blockX = (width_ - blockW) * 0.5f;

            curY -= blockH + kBlockGap;

            float opacity = b.opacity;

            bgBrush_->SetOpacity(opacity * 0.70f);
            D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(
                D2D1::RectF(blockX, curY, blockX + blockW, curY + blockH),
                kCornerRadius, kCornerRadius);
            rt_->FillRoundedRectangle(rrect, bgBrush_.Get());

            labelBrush_->SetOpacity(opacity * 0.8f);
            D2D1_RECT_F labelRect = D2D1::RectF(
                blockX + kBlockPadH, curY + kBlockPadV,
                blockX + blockW - kBlockPadH, curY + kBlockPadV + labelH);
            rt_->DrawText(b.label.c_str(), static_cast<UINT32>(b.label.size()),
                          labelFormat_.Get(), labelRect, labelBrush_.Get());

            textBrush_->SetOpacity(opacity);
            float textY = curY + kBlockPadV + labelH;
            D2D1_POINT_2F origin{blockX + kBlockPadH, textY};

            ComPtr<IDWriteTextLayout> drawLayout;
            dwFactory_->CreateTextLayout(
                b.text.c_str(), static_cast<UINT32>(b.text.size()),
                textFormat_.Get(), blockW - kBlockPadH * 2, textH,
                drawLayout.GetAddressOf());
            if (drawLayout) {
                rt_->DrawTextLayout(origin, drawLayout.Get(), textBrush_.Get());
            }
        }
    }

    hr = rt_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        discardDeviceResources();
        SelectObject(memDC, oldBmp);
        DeleteObject(hBmp);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    // Commit to screen via UpdateLayeredWindow
    RECT winRect;
    GetWindowRect(hwnd_, &winRect);
    POINT ptPos{winRect.left, winRect.top};
    SIZE  sizeWnd{width_, height_};
    POINT ptSrc{0, 0};
    BLENDFUNCTION blend{};
    blend.BlendOp             = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat         = AC_SRC_ALPHA;

    UpdateLayeredWindow(hwnd_, screenDC, &ptPos, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void Renderer::updateLayered(HWND, int, int) {
    // Handled inline in render() for single-pass efficiency
}
