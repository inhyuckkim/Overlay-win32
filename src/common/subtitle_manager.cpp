#include "subtitle_manager.h"
#include <algorithm>
#include <chrono>
#include <cstdint>

static uint64_t nowMsSinceEpoch() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

namespace {
size_t utf8CharCount(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0u) != 0x80u) ++count;
    }
    return count;
}

std::string utf8TrimFront(const std::string& s, size_t keepChars) {
    const size_t totalChars = utf8CharCount(s);
    if (totalChars <= keepChars) return s;

    const size_t skipChars = totalChars - keepChars;
    size_t skipped = 0;
    size_t bytePos = 0;

    while (bytePos < s.size() && skipped < skipChars) {
        unsigned char c = static_cast<unsigned char>(s[bytePos]);
        if ((c & 0xC0u) != 0x80u) ++skipped;
        ++bytePos;
    }

    while (bytePos < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[bytePos]);
        if ((c & 0xC0u) != 0x80u) break;
        ++bytePos;
    }

    return s.substr(bytePos);
}

uint64_t durationForText(size_t charCount) {
    uint64_t duration = static_cast<uint64_t>(SubtitleManager::kDurationBaseMs) +
                        static_cast<uint64_t>(charCount) * SubtitleManager::kDurationPerCharMs;
    duration = (std::max)(duration, static_cast<uint64_t>(SubtitleManager::kMinDurationMs));
    duration = (std::min)(duration, static_cast<uint64_t>(SubtitleManager::kMaxDurationMs));
    return duration;
}
} // namespace

void SubtitleManager::addLanguage(const std::string& code, const std::string& label) {
    if (findSlot(code)) return;
    if (static_cast<int>(slots_.size()) >= kMaxLanguages) return;

    LanguageSlot slot;
    slot.langCode         = code;
    slot.label            = label;
    slot.visible          = true;
    slot.lastUpdateTickMs = nowMsSinceEpoch();
    slot.lastFinalAtMs    = 0;
    slot.hideAtMs         = 0;
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

    const uint64_t now      = nowMsSinceEpoch();
    const size_t   newChars = utf8CharCount(text);

    if (isFinal) {
        const bool hasPrevFinal = !slot->finalText.empty();
        const bool isSmallGap   = (now - slot->lastFinalAtMs) <= kAppendGapMs;
        const bool isShortFinal = newChars <= kAppendMaxChars;

        if (hasPrevFinal && isSmallGap && isShortFinal) {
            if (!slot->finalText.empty() && !text.empty()) {
                slot->finalText += " ";
            }
            slot->finalText += text;

            size_t combinedChars = utf8CharCount(slot->finalText);
            if (combinedChars > kMaxCombinedChars) {
                slot->finalText = utf8TrimFront(slot->finalText, kMaxCombinedChars);
                combinedChars   = kMaxCombinedChars;
            }
            slot->hideAtMs = now + durationForText(combinedChars);
        } else {
            slot->finalText = text;
            slot->hideAtMs  = now + durationForText(newChars);
        }

        slot->interimText.clear();
        slot->lastFinalAtMs = now;
    } else {
        const bool finalStillVisible = !slot->finalText.empty() && slot->hideAtMs > now;
        const bool shortInterim      = newChars <= kAppendMaxChars;
        if (finalStillVisible && shortInterim) return;

        slot->interimText = text;
        slot->hideAtMs    = now + kInterimBaselineMs;
    }

    slot->visible          = true;
    slot->lastUpdateTickMs = now;
}

void SubtitleManager::updateTranslation(const std::string& targetLang,
                                        const std::string& text,
                                        bool isFinal) {
    LanguageSlot* slot = findSlot(targetLang);
    if (!slot) return;

    const uint64_t now      = nowMsSinceEpoch();
    const size_t   newChars = utf8CharCount(text);

    if (isFinal) {
        const bool hasPrevFinal = !slot->finalText.empty();
        const bool isSmallGap   = (now - slot->lastFinalAtMs) <= kAppendGapMs;
        const bool isShortFinal = newChars <= kAppendMaxChars;

        if (hasPrevFinal && isSmallGap && isShortFinal) {
            if (!slot->finalText.empty() && !text.empty()) {
                slot->finalText += " ";
            }
            slot->finalText += text;

            size_t combinedChars = utf8CharCount(slot->finalText);
            if (combinedChars > kMaxCombinedChars) {
                slot->finalText = utf8TrimFront(slot->finalText, kMaxCombinedChars);
                combinedChars   = kMaxCombinedChars;
            }
            slot->hideAtMs = now + durationForText(combinedChars);
        } else {
            slot->finalText = text;
            slot->hideAtMs  = now + durationForText(newChars);
        }

        slot->interimText.clear();
        slot->lastFinalAtMs = now;
    } else {
        const bool finalStillVisible = !slot->finalText.empty() && slot->hideAtMs > now;
        const bool shortInterim      = newChars <= kAppendMaxChars;
        if (finalStillVisible && shortInterim) return;

        slot->interimText = text;
        slot->hideAtMs    = now + kInterimBaselineMs;
    }

    slot->visible          = true;
    slot->lastUpdateTickMs = now;
}

void SubtitleManager::reset() {
    slots_.clear();
}

void SubtitleManager::tick() {
    const uint64_t now = nowMsSinceEpoch();
    for (auto& slot : slots_) {
        if (slot.visible && slot.hideAtMs > 0 && now >= slot.hideAtMs) {
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
            block.opacity   = 0.70f;
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
