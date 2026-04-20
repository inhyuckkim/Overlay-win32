#pragma once
#include <cstdint>
#include <string>

struct SubtitleBlock {
    std::string label;
    std::string text;
    float       opacity = 1.0f;
    bool        isInterim = false;
};

struct LanguageSlot {
    std::string langCode;
    std::string label;
    std::string interimText;
    std::string finalText;
    bool        visible = true;
    uint64_t    lastUpdateTickMs = 0;
    uint64_t    lastFinalAtMs = 0;
    uint64_t    hideAtMs = 0;
};
