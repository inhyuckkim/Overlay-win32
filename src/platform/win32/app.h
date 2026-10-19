#pragma once
#include <memory>
#include <vector>
#include <windows.h>

class OverlayWindow;
class Renderer;
class SubtitleManager;
class WsClient;
class MessageHandler;
class MenuButton;
class TestInput;

struct OverlayColors;

/** One connected monitor, in virtual-desktop pixels. */
struct MonitorRects {
    RECT full{};     ///< Whole monitor (MONITORINFO::rcMonitor).
    RECT work{};     ///< Work area, i.e. taskbar excluded (MONITORINFO::rcWork).
    bool primary = false;
};

/**
 * Enumerate connected monitors ordered by rect (left, top) ascending.
 *
 * IMPORTANT: the Lixor Host server sorts its /api/overlay/displays result by the
 * exact same key, so a given 0-based index names the same physical screen on
 * both sides. Do not change one ordering without changing the other.
 */
std::vector<MonitorRects> enumerateMonitorsSorted();

class App {
public:
    static App& instance();

    bool init(HINSTANCE hInstance);
    int  run();
    void shutdown();

    HINSTANCE        hInstance() const { return hInstance_; }
    OverlayWindow*   overlayWindow() const { return overlayWindow_.get(); }
    Renderer*        renderer() const { return renderer_.get(); }
    SubtitleManager* subtitleManager() const { return subtitleManager_.get(); }
    WsClient*        wsClient() const { return wsClient_.get(); }
    MessageHandler*  messageHandler() const { return messageHandler_.get(); }
    MenuButton*      menuButton() const { return menuButton_.get(); }
    TestInput*       testInput() const { return testInput_.get(); }

    void requestRedraw();

    void setFontSizeLevel(int level);
    void setColors(const OverlayColors& colors);
    /** 0-based index into enumerateMonitorsSorted(); out-of-range falls back to 0. */
    void setDisplayIndex(int index);
    /** Re-enumerate after WM_DISPLAYCHANGE / work-area change and re-place the windows. */
    void handleDisplayChange();

private:
    App() = default;

    /** Move the overlay + menu button onto monitors[displayIndex_]. */
    void applyDisplayPlacement();

    HINSTANCE hInstance_ = nullptr;
    /** Currently selected monitor, 0-based in enumerateMonitorsSorted() order. */
    int       displayIndex_ = 0;

    std::unique_ptr<OverlayWindow>   overlayWindow_;
    std::unique_ptr<Renderer>        renderer_;
    std::unique_ptr<SubtitleManager> subtitleManager_;
    std::unique_ptr<WsClient>        wsClient_;
    std::unique_ptr<MessageHandler>  messageHandler_;
    std::unique_ptr<MenuButton>      menuButton_;
    std::unique_ptr<TestInput>       testInput_;
};
