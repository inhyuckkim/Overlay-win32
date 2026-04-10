#include "menu_button.h"
#import <Cocoa/Cocoa.h>

#include "app.h"
#include "subtitle_manager.h"

#include <string>

@interface LixorMenuController : NSObject
- (void)menuToggle:(id)sender;
- (void)menuTest:(id)sender;
- (void)menuReset:(id)sender;
- (void)menuQuit:(id)sender;
@end

@implementation LixorMenuController

- (void)menuToggle:(id)sender {
    (void)sender;
    auto* mgr = App::instance().subtitleManager();
    if (mgr) {
        mgr->setVisible(!mgr->isVisible());
        App::instance().requestRedraw();
    }
}

- (void)menuTest:(id)sender {
    (void)sender;
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Test Subtitle";
    alert.informativeText = @"Enter text to display on the overlay:";

    NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
    [input setStringValue:@""];
    alert.accessoryView = input;
    [alert addButtonWithTitle:@"Send"];
    [alert addButtonWithTitle:@"Cancel"];

    NSModalResponse r = [alert runModal];
    if (r == NSAlertFirstButtonReturn) {
        [input validateEditing];
        NSString* ns = [input stringValue];
        if (ns.length == 0) return;

        std::string text([ns UTF8String]);

        auto* mgr = App::instance().subtitleManager();
        if (mgr) {
            mgr->addLanguage("test", "Test");
            mgr->updateSubtitle("test", text, true);
            App::instance().requestRedraw();
        }
    }
}

- (void)menuReset:(id)sender {
    (void)sender;
    auto* mgr = App::instance().subtitleManager();
    if (mgr) {
        mgr->reset();
        App::instance().requestRedraw();
    }
}

- (void)menuQuit:(id)sender {
    (void)sender;
    [NSApp terminate:nil];
}

@end

struct MenuButton::Impl {
    NSStatusItem*        item       = nil;
    LixorMenuController* controller = nil;
};

MenuButton::MenuButton() : impl_(std::make_unique<Impl>()) {}

MenuButton::~MenuButton() {
    destroy();
}

bool MenuButton::create() {
    impl_->controller = [[LixorMenuController alloc] init];

    NSStatusBar* bar = [NSStatusBar systemStatusBar];
    NSStatusItem* item = [bar statusItemWithLength:NSVariableStatusItemLength];
    impl_->item = item;

    [item.button setTitle:@"L"];
    [item.button setToolTip:@"Lixor Overlay"];

    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* toggle = [[NSMenuItem alloc] initWithTitle:@"Toggle Subtitles"
                                                    action:@selector(menuToggle:)
                                             keyEquivalent:@""];
    toggle.target = impl_->controller;

    NSMenuItem* test = [[NSMenuItem alloc] initWithTitle:@"Test Subtitle..."
                                                  action:@selector(menuTest:)
                                           keyEquivalent:@""];
    test.target = impl_->controller;

    NSMenuItem* reset = [[NSMenuItem alloc] initWithTitle:@"Reset"
                                                   action:@selector(menuReset:)
                                            keyEquivalent:@""];
    reset.target = impl_->controller;

    NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                  action:@selector(menuQuit:)
                                           keyEquivalent:@"q"];
    quit.target = impl_->controller;

    [menu addItem:toggle];
    [menu addItem:test];
    [menu addItem:reset];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:quit];

    item.menu = menu;
    return true;
}

void MenuButton::destroy() {
    if (impl_->item) {
        [[NSStatusBar systemStatusBar] removeStatusItem:impl_->item];
        impl_->item = nil;
    }
    impl_->controller = nil;
}
