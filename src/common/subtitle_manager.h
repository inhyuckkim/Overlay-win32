#pragma once
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SubtitleManager {
public:
    static constexpr int     kMaxLanguages    = 4;
    static constexpr uint32_t kAppendGapMs = 3500;
    static constexpr size_t   kAppendMaxChars = 15;
    static constexpr size_t   kMaxCombinedChars = 120;
    static constexpr uint32_t kMinDurationMs = 3500;
    static constexpr uint32_t kMaxDurationMs = 14000;
    static constexpr uint32_t kDurationBaseMs = 2500;
    static constexpr uint32_t kDurationPerCharMs = 110;
    static constexpr uint32_t kInterimBaselineMs = 4000;
    static constexpr uint32_t kTimerIntervalMs = 500;
    static constexpr int     kTimerId         = 100;

    void addLanguage(const std::string& code, const std::string& label);
    void removeLanguage(const std::string& code);
    void updateSubtitle(const std::string& langCode, const std::string& text, bool isFinal);
    void updateTranslation(const std::string& targetLang, const std::string& text, bool isFinal);
    void reset();
    void tick();

    bool isVisible() const { return overlayVisible_; }
    void setVisible(bool v) { overlayVisible_ = v; }
    int languageCount() const { return static_cast<int>(slots_.size()); }

    std::vector<SubtitleBlock> getVisibleBlocks() const;

private:
    LanguageSlot* findSlot(const std::string& code);

    std::vector<LanguageSlot> slots_;
    bool                      overlayVisible_ = true;
};
