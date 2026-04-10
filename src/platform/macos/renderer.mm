#include "renderer.h"
#import <Cocoa/Cocoa.h>

#include <algorithm>
#include <cmath>

static NSString* toNSString(const std::string& s) {
    if (s.empty()) return @"";
    return [[NSString alloc] initWithBytes:s.data()
                                      length:s.size()
                                    encoding:NSUTF8StringEncoding];
}

static NSSize measureTextBlock(NSString* text, NSFont* font, CGFloat maxWidth) {
    if (!text.length) return NSMakeSize(0, 0);
    NSDictionary* attrs = @{ NSFontAttributeName: font };
    NSSize box = NSMakeSize(maxWidth, CGFLOAT_MAX);
    NSStringDrawingOptions opts = NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading;
    NSRect r = [text boundingRectWithSize:box options:opts attributes:attrs context:nil];
    return r.size;
}

bool Renderer::init() {
    updateFonts();
    return true;
}

void Renderer::release() {}

void Renderer::updateFonts() {
    fontSize_  = 12.0f + static_cast<float>(fontSizeLevel_ - 1) * (20.0f / 9.0f);
    labelSize_ = fontSize_ * (13.0f / 22.0f);
}

void Renderer::setFontSizeLevel(int level) {
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    if (fontSizeLevel_ == level) return;
    fontSizeLevel_ = level;
    updateFonts();
}

void Renderer::resize(int width, int height) {
    width_  = width;
    height_ = height;
}

int Renderer::measureHeight(const std::vector<SubtitleBlock>& blocks) const {
    if (blocks.empty() || width_ <= 0) return 0;

    NSFont* textFont = [NSFont systemFontOfSize:static_cast<CGFloat>(fontSize_) weight:NSFontWeightSemibold];
    float   maxTextWidth = static_cast<float>(width_) * 0.85f;
    float   totalH       = 0.0f;
    int     count        = 0;

    for (const auto& b : blocks) {
        if (b.text.empty()) continue;

        NSString* nsText = toNSString(b.text);
        NSSize    sz     = measureTextBlock(nsText, textFont, maxTextWidth);
        float     lineH  = fontSize_ * 1.3f;
        float     textH  = (std::min)(static_cast<float>(sz.height) + 4.0f, lineH * 4.0f);
        float     labelH = labelSize_ + 4.0f;
        float     blockH = kBlockPadV + labelH + textH + kBlockPadV;

        if (count > 0) totalH += kBlockGap;
        totalH += blockH;
        ++count;
    }

    return count > 0 ? static_cast<int>(totalH + 0.5f) : 0;
}

void Renderer::render(const std::vector<SubtitleBlock>& blocks) {
    NSGraphicsContext* nsctx = [NSGraphicsContext currentContext];
    if (!nsctx) return;

    [NSGraphicsContext saveGraphicsState];

    NSRect bounds = NSMakeRect(0, 0, width_, height_);
    [[NSColor clearColor] set];
    NSRectFill(bounds);

    if (blocks.empty()) {
        [NSGraphicsContext restoreGraphicsState];
        return;
    }

    NSFont* fontText  = [NSFont systemFontOfSize:static_cast<CGFloat>(fontSize_) weight:NSFontWeightSemibold];
    NSFont* fontLabel = [NSFont systemFontOfSize:static_cast<CGFloat>(labelSize_) weight:NSFontWeightRegular];

    float maxTextWidth = static_cast<float>(width_) * 0.85f;
    float curY         = static_cast<float>(height_);

    for (const auto& b : blocks) {
        if (b.text.empty()) continue;

        NSString* nsText = toNSString(b.text);
        NSSize    tsize  = measureTextBlock(nsText, fontText, maxTextWidth);
        float     lineH  = fontSize_ * 1.3f;
        float     textH  = (std::min)(static_cast<float>(tsize.height) + 4.0f, lineH * 4.0f);

        float labelH = labelSize_ + 4.0f;
        float blockH = kBlockPadV + labelH + textH + kBlockPadV;

        NSDictionary* measureAttrs = @{ NSFontAttributeName: fontText };
        NSSize fullSz = [nsText boundingRectWithSize:NSMakeSize(maxTextWidth, CGFLOAT_MAX)
                                             options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading
                                          attributes:measureAttrs
                                             context:nil]
                            .size;

        float blockW = (std::min)(static_cast<float>(fullSz.width) + kBlockPadH * 2, static_cast<float>(width_));
        blockW       = (std::max)(blockW, 200.0f);
        float blockX = (static_cast<float>(width_) - blockW) * 0.5f;

        curY -= blockH + kBlockGap;

        float opacity = b.opacity;

        [[NSColor colorWithCalibratedWhite:0.0f alpha:static_cast<CGFloat>(opacity * 0.70)] setFill];
        NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(blockX, curY, blockW, blockH)
                                                             xRadius:kCornerRadius
                                                             yRadius:kCornerRadius];
        [path fill];

        NSString* nsLabel = toNSString(b.label);
        NSDictionary* labelAttrs = @{
            NSFontAttributeName : fontLabel,
            NSForegroundColorAttributeName : [NSColor colorWithCalibratedWhite:0.7f alpha:static_cast<CGFloat>(opacity * 0.8f)]
        };
        NSSize labelSize = [nsLabel sizeWithAttributes:labelAttrs];
        (void)labelSize;
        NSRect labelRect = NSMakeRect(blockX + kBlockPadH, curY + kBlockPadV, blockW - kBlockPadH * 2, labelH);
        [nsLabel drawWithRect:labelRect
                      options:NSStringDrawingUsesLineFragmentOrigin
                   attributes:labelAttrs
                      context:nil];

        NSDictionary* textAttrs = @{
            NSFontAttributeName : fontText,
            NSForegroundColorAttributeName : [NSColor colorWithCalibratedWhite:1.0f alpha:opacity]
        };
        NSRect textRect = NSMakeRect(blockX + kBlockPadH, curY + kBlockPadV + labelH, maxTextWidth, textH);
        [nsText drawWithRect:textRect
                     options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading
                  attributes:textAttrs
                     context:nil];
    }

    [NSGraphicsContext restoreGraphicsState];
}
