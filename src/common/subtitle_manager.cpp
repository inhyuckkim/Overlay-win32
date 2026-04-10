#include "subtitle_manager.h"
#include <algorithm>
#include <chrono>

static uint64_t nowMsSinceEpoch() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void SubtitleManager::addLanguage(const std::string& code, const std::string& label) {
    if (findSlot(code)) return;
    if (static_cast<int>(slots_.size()) >= kMaxLanguages) return;

    LanguageSlot slot;
    slot.langCode         = code;
    slot.label            = label;
    slot.visible          = true;
    slot.lastUpdateTickMs = nowMsSinceEpoch();
    slots_.push_back(std::move(slot));
}

void SubtitleManager::removeLanguage(const std::string& code) {
    slots_.erase(
        std::remove_if(slots_.begin(), slots_.end(),
                       [&](const LanguageSlot& s) { return s.langCode == code; }),
        slots_.end());
}

void SubtitleManager::updateSubtitle(const std::string& langCode,
                                     const std::string& text,
                                     bool isFinal) {
    LanguageSlot* slot = findSlot(langCode);
    if (!slot) return;

    if (isFinal) {
        slot->finalText = text;
        slot->interimText.clear();
    } else {
        slot->interimText = text;
    }
    slot->visible          = true;
    slot->lastUpdateTickMs = nowMsSinceEpoch();
}

void SubtitleManager::updateTranslation(const std::string& targetLang,
                                        const std::string& text) {
    LanguageSlot* slot = findSlot(targetLang);
    if (!slot) return;

    slot->finalText      = text;
    slot->interimText.clear();
    slot->visible        = true;
    slot->lastUpdateTickMs = nowMsSinceEpoch();
}

void SubtitleManager::reset() {
    slots_.clear();
}

void SubtitleManager::tick() {
    const uint64_t now = nowMsSinceEpoch();
    for (auto& slot : slots_) {
        if (slot.visible && (now - slot.lastUpdateTickMs) > kAutoHideMs) {
            slot.visible = false;
        }
    }
}

std::vector<SubtitleBlock> SubtitleManager::getVisibleBlocks() const {
    std::vector<SubtitleBlock> out;
    if (!overlayVisible_) return out;

    for (const auto& slot : slots_) {
        if (!slot.visible) continue;

        SubtitleBlock block;
        block.label = slot.label;

        if (!slot.interimText.empty()) {
            block.text      = slot.interimText;
            block.opacity   = 0.55f;
            block.isInterim = true;
        } else if (!slot.finalText.empty()) {
            block.text      = slot.finalText;
            block.opacity   = 1.0f;
            block.isInterim = false;
        } else {
            continue;
        }

        out.push_back(std::move(block));
    }
    return out;
}

LanguageSlot* SubtitleManager::findSlot(const std::string& code) {
    for (auto& s : slots_) {
        if (s.langCode == code) return &s;
    }
    return nullptr;
}
