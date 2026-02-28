#include "message_handler.h"
#include "app.h"
#include "subtitle_manager.h"
#include <windows.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

static std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), len);
    return out;
}

MessageHandler::MessageHandler(SubtitleManager* mgr, App* app) : mgr_(mgr), app_(app) {}

void MessageHandler::dispatch(const std::string& raw) {
    try {
        auto j = json::parse(raw);
        std::string type = j.value("type", "");

        if (type == "subtitle") {
            std::wstring lang = utf8ToWide(j.value("language", ""));
            std::wstring text = utf8ToWide(j.value("text", ""));
            bool isFinal      = j.value("isFinal", false);
            mgr_->updateSubtitle(lang, text, isFinal);

        } else if (type == "translation") {
            std::wstring targetLang = utf8ToWide(j.value("targetLanguage", ""));
            std::wstring text       = utf8ToWide(j.value("text", ""));
            mgr_->updateTranslation(targetLang, text);

        } else if (type == "add_language") {
            std::wstring lang  = utf8ToWide(j.value("language", ""));
            std::wstring label = utf8ToWide(j.value("label", ""));
            if (label.empty()) label = lang;
            mgr_->addLanguage(lang, label);

        } else if (type == "remove_language") {
            std::wstring lang = utf8ToWide(j.value("language", ""));
            mgr_->removeLanguage(lang);

        } else if (type == "show") {
            mgr_->setVisible(true);

        } else if (type == "hide") {
            mgr_->setVisible(false);

        } else if (type == "reset") {
            mgr_->reset();

        } else if (type == "font_size" && app_) {
            int level = j.value("level", 5);
            if (level < 1) level = 1;
            if (level > 10) level = 10;
            app_->setFontSizeLevel(level);
        }
    } catch (const json::exception&) {
        OutputDebugStringA("[MessageHandler] JSON parse error\n");
    }
}
