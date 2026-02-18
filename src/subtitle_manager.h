#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "renderer.h"

struct LanguageSlot {
    std::wstring langCode;
    std::wstring label;
    std::wstring interimText;
    std::wstring finalText;
    bool         visible = true;
    ULONGLONG    lastUpdateTick = 0;
};

class SubtitleManager {
public:
    static constexpr int    kMaxLanguages   = 4;
    static constexpr DWORD  kAutoHideMs     = 5000;
    static constexpr DWORD  kTimerIntervalMs = 500;
    static constexpr UINT   kTimerId        = 100;

    void addLanguage(const std::wstring& code, const std::wstring& label);
    void removeLanguage(const std::wstring& code);
    void updateSubtitle(const std::wstring& langCode, const std::wstring& text, bool isFinal);
    void updateTranslation(const std::wstring& targetLang, const std::wstring& text);
    void reset();
    void tick();

    bool isVisible() const { return overlayVisible_; }
    void setVisible(bool v) { overlayVisible_ = v; }

    std::vector<SubtitleBlock> getVisibleBlocks() const;

private:
    LanguageSlot* findSlot(const std::wstring& code);

    std::vector<LanguageSlot> slots_;
    bool overlayVisible_ = true;
};
