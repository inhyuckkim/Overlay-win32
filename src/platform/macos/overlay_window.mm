#include "overlay_window.h"
#import <Cocoa/Cocoa.h>

#include "app.h"
#include "renderer.h"
#include "subtitle_manager.h"

@interface LixorOverlayView : NSView
@end

@implementation LixorOverlayView
- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    App& app = App::instance();
    auto*  mgr = app.subtitleManager();
    auto*  ren = app.renderer();
    if (!mgr || !ren) return;
    auto blocks = mgr->getVisibleBlocks();
    ren->render(blocks);
}
@end

struct OverlayWindow::Impl {
    NSPanel*         panel  = nil;
    LixorOverlayView* view = nil;
    int              width  = 0;
    int              height = 0;
    int              overlayBottomY = 0;
};

OverlayWindow::OverlayWindow() : impl_(std::make_unique<Impl>()) {}

OverlayWindow::~OverlayWindow() {
    destroy();
}

bool OverlayWindow::create(int screenW, int overlayBottomY) {
    impl_->overlayBottomY = overlayBottomY;
    impl_->width          = screenW;
    static constexpr int kInitialH = 480;
    impl_->height         = kInitialH;

    NSRect vf   = [[NSScreen mainScreen] visibleFrame];
    CGFloat x   = NSMinX(vf);
    CGFloat y   = static_cast<CGFloat>(overlayBottomY);
    CGFloat w   = static_cast<CGFloat>(screenW);
    CGFloat ih  = static_cast<CGFloat>(kInitialH);
    NSRect contentRect = NSMakeRect(x, y, w, ih);

    NSPanel* panel = [[NSPanel alloc] initWithContentRect:contentRect
                                                styleMask:NSWindowStyleMaskBorderless
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
    impl_->panel = panel;

    [panel setOpaque:NO];
    [panel setBackgroundColor:[NSColor clearColor]];
    // Above typical document windows so a click in another app does not bury the overlay behind it.
    [panel setLevel:NSStatusWindowLevel];
    [panel setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                 NSWindowCollectionBehaviorFullScreenAuxiliary];
    [panel setIgnoresMouseEvents:YES];
    [panel setHasShadow:NO];
    [panel setReleasedWhenClosed:NO];
    // Default NSPanel behavior: hide when this app is no longer active. LixorOverlay is an
    // accessory app; clicking the browser activates Chrome/Safari and we resign active — the
    // panel would disappear immediately even though subtitles should stay. HUD-style overlay
    // must remain visible across app activation changes.
    [panel setHidesOnDeactivate:NO];

    LixorOverlayView* view = [[LixorOverlayView alloc] initWithFrame:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)];
    impl_->view = view;
    [panel setContentView:view];

    return true;
}

void OverlayWindow::destroy() {
    if (impl_->panel) {
        [impl_->panel close];
        impl_->panel = nil;
        impl_->view  = nil;
    }
}

void OverlayWindow::show() {
    if (impl_->panel) {
        [impl_->panel orderFrontRegardless];
        reassertTopmost();
    }
}

void OverlayWindow::hide() {
    if (impl_->panel)
        [impl_->panel orderOut:nil];
}

void OverlayWindow::reassertTopmost() {
    if (impl_->panel) {
        [impl_->panel setLevel:NSStatusWindowLevel];
        [impl_->panel orderFrontRegardless];
    }
}

void OverlayWindow::setHeight(int h) {
    impl_->height = (h > 0 ? h : 0);
    if (!impl_->panel) return;

    CGFloat w      = static_cast<CGFloat>(impl_->width);
    CGFloat height = static_cast<CGFloat>(impl_->height);
    NSRect  f      = [impl_->panel frame];
    NSRect frame   = NSMakeRect(f.origin.x, f.origin.y, w, height);
    [impl_->panel setFrame:frame display:YES];
    if (impl_->view) {
        [impl_->view setFrame:NSMakeRect(0, 0, w, height)];
    }
}

int OverlayWindow::width() const {
    return impl_->width;
}

int OverlayWindow::height() const {
    return impl_->height;
}

void OverlayWindow::invalidate() {
    if (impl_->view)
        [impl_->view setNeedsDisplay:YES];
}
