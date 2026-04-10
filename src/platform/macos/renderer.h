#pragma once
#include "types.h"
#include <vector>

class Renderer {
public:
    bool init();
    void release();
    void resize(int width, int height);
    void render(const std::vector<SubtitleBlock>& blocks);

    void setFontSizeLevel(int level);

    int measureHeight(const std::vector<SubtitleBlock>& blocks) const;

private:
    void updateFonts();

    int   width_         = 0;
    int   height_        = 0;
    int   fontSizeLevel_ = 5;
    float fontSize_      = 22.0f;
    float labelSize_     = 13.0f;

    static constexpr float kBlockPadH    = 18.0f;
    static constexpr float kBlockPadV    = 10.0f;
    static constexpr float kBlockGap     = 6.0f;
    static constexpr float kCornerRadius = 8.0f;
};
