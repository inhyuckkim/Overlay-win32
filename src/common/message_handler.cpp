#include "message_handler.h"
#include "subtitle_manager.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
        }
    } catch (const json::exception&) {
    }
}
