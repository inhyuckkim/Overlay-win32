#include "message_handler.h"
#include "subtitle_manager.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Parse "#RRGGBB" (or "RRGGBB") into three 0.0-1.0 channels.
 * Returns false and leaves the outputs untouched on any malformed input, so the
 * caller keeps whatever default it already had.
 */
static bool parseHexColor(const std::string& hex, float& r, float& g, float& b) {
    std::string s = hex;
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 6) return false;
    unsigned int v = 0;
    for (char c : s) {
        unsigned int d;
        if      (c >= '0' && c <= '9') d = static_cast<unsigned int>(c - '0');
        else if (c >= 'a' && c <= 'f') d = static_cast<unsigned int>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = static_cast<unsigned int>(c - 'A' + 10);
        else return false;
        v = (v << 4) | d;
    }
    r = static_cast<float>((v >> 16) & 0xFF) / 255.0f;
    g = static_cast<float>((v >> 8)  & 0xFF) / 255.0f;
    b = static_cast<float>( v        & 0xFF) / 255.0f;
    return true;
}

MessageHandler::MessageHandler(SubtitleManager* mgr, AppCallbacks callbacks)
    : mgr_(mgr), callbacks_(std::move(callbacks)) {}

void MessageHandler::dispatch(const std::string& raw) {
    try {
        auto j    = json::parse(raw);
        std::string type = j.value("type", "");

        if (type == "subtitle") {
            std::string lang = j.value("language", "");
            std::string text = j.value("text", "");
            bool isFinal     = j.value("isFinal", false);
            mgr_->updateSubtitle(lang, text, isFinal);

        } else if (type == "translation") {
            std::string targetLang = j.value("targetLanguage", "");
            std::string text       = j.value("text", "");
            bool isFinal           = j.value("isFinal", true);
            mgr_->updateTranslation(targetLang, text, isFinal);

        } else if (type == "add_language") {
            std::string lang  = j.value("language", "");
            std::string label = j.value("label", "");
            if (label.empty()) label = lang;
            mgr_->addLanguage(lang, label);

        } else if (type == "remove_language") {
            std::string lang = j.value("language", "");
            mgr_->removeLanguage(lang);

        } else if (type == "show") {
            mgr_->setVisible(true);

        } else if (type == "hide") {
            mgr_->setVisible(false);

        } else if (type == "reset") {
            mgr_->reset();

        } else if (type == "font_size") {
            int level = j.value("level", 5);
            if (level < 1) level = 1;
            if (level > 10) level = 10;
            if (callbacks_.setFontSizeLevel) callbacks_.setFontSizeLevel(level);

        } else if (type == "colors") {
            // Start from the current defaults; each field that fails to parse is left alone.
            OverlayColors c;
            parseHexColor(j.value("textColor", std::string()), c.textR, c.textG, c.textB);
            parseHexColor(j.value("bgColor", std::string()), c.bgR, c.bgG, c.bgB);
            if (j.contains("bgOpacity") && j["bgOpacity"].is_number()) {
                float a = j["bgOpacity"].get<float>();
                if (a < 0.0f) a = 0.0f;
                if (a > 1.0f) a = 1.0f;
                c.bgOpacity = a;
            }
            if (callbacks_.setColors) callbacks_.setColors(c);

        } else if (type == "set_display") {
            // 0-based, ordered by monitor rect (left, top) ascending - the host uses
            // the same ordering so "monitor 2" means the same screen on both sides.
            int index = j.value("index", 0);
            if (index < 0) index = 0;
            if (callbacks_.setDisplayIndex) callbacks_.setDisplayIndex(index);
        }
    } catch (const json::exception&) {
    }
}
