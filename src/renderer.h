#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

struct SubtitleBlock {
    std::wstring label;
    std::wstring text;
    float opacity;      // 0.0 – 1.0
    bool  isInterim;
};

class Renderer {
public:
    bool init(HWND hwnd, int width, int height);
    void release();
    void resize(int width, int height);
    void render(const std::vector<SubtitleBlock>& blocks);

    void setFontSizeLevel(int level);

private:
    void createDeviceResources();
    void discardDeviceResources();
    void createTextFormats();
    void updateLayered(HWND hwnd, int width, int height);

    HWND hwnd_ = nullptr;
    int  width_  = 0;
    int  height_ = 0;

    int   fontSizeLevel_ = 5;
    float fontSize_      = 22.0f;
    float labelSize_     = 13.0f;

    ComPtr<ID2D1Factory>             d2dFactory_;
    ComPtr<IDWriteFactory>           dwFactory_;
    ComPtr<ID2D1DCRenderTarget>      rt_;
    ComPtr<IDWriteTextFormat>        textFormat_;
    ComPtr<IDWriteTextFormat>        labelFormat_;
    ComPtr<ID2D1SolidColorBrush>     textBrush_;
    ComPtr<ID2D1SolidColorBrush>     bgBrush_;
    ComPtr<ID2D1SolidColorBrush>     labelBrush_;

    static constexpr float kBlockPadH   = 18.0f;
    static constexpr float kBlockPadV   = 10.0f;
    static constexpr float kBlockGap    = 6.0f;
    static constexpr float kCornerRadius = 8.0f;
};
