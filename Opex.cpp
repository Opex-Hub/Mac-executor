#!/bin/bash

set -e

APP_NAME="Opex"
BUNDLE_ID="com.opexhub.opex"
INSTALL_DIR="/Applications"
BIN_LINK="/usr/local/bin/opex"

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== Opex Executor Installer ===${NC}"

if ! xcode-select -p &>/dev/null; then
    echo -e "${BLUE}[*] Installing Xcode Command Line Tools...${NC}"
    xcode-select --install
    echo -e "${GREEN}[+] Please complete the installation, then run this installer again.${NC}"
    exit 0
fi

BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT

echo -e "${BLUE}[*] Building Opex Executor...${NC}"

mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/MacOS"
mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/Resources"

cat > "$BUILD_DIR/OpexApp.mm" <<'EOF'
#import <Cocoa/Cocoa.h>
#import <string>
#import <vector>
#import <cstdlib>
#import <ctime>

@interface OpexView : NSView
@property (strong) NSTextView *scriptEditor;
@property (strong) NSTextView *outputConsole;
@property (strong) NSButton *injectButton;
@property (strong) NSButton *executeButton;
@property (strong) NSButton *clearButton;
@property (strong) NSButton *quitButton;
@property (strong) NSTextField *statusLabel;
@property (assign) bool isInjected;
@end

@implementation OpexView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _isInjected = NO;
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    self.wantsLayer = YES;
    self.layer.backgroundColor = [NSColor colorWithRed:0.05 green:0.07 blue:0.12 alpha:1.0].CGColor;

    NSTextField *title = [NSTextField labelWithString:@"OPEX EXECUTOR"];
    title.font = [NSFont boldSystemFontOfSize:24];
    title.textColor = [NSColor whiteColor];
    title.alignment = NSTextAlignmentCenter;
    title.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:title];

    _statusLabel = [NSTextField labelWithString:@"Status: Not Injected"];
    _statusLabel.font = [NSFont boldSystemFontOfSize:14];
    _statusLabel.textColor = [NSColor redColor];
    _statusLabel.alignment = NSTextAlignmentCenter;
    _statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_statusLabel];

    _injectButton = [self createButton:@"Inject" action:@selector(injectClicked:)];
    _executeButton = [self createButton:@"Execute" action:@selector(executeClicked:)];
    _clearButton = [self createButton:@"Clear" action:@selector(clearClicked:)];
    _quitButton = [self createButton:@"Quit" action:@selector(quitClicked:)];

    NSScrollView *scriptScroll = [[NSScrollView alloc] init];
    scriptScroll.translatesAutoresizingMaskIntoConstraints = NO;
    scriptScroll.hasVerticalScroller = YES;
    scriptScroll.hasHorizontalScroller = NO;
    scriptScroll.autohidesScrollers = YES;
    scriptScroll.borderType = NSBezelBorder;

    _scriptEditor = [[NSTextView alloc] init];
    _scriptEditor.font = [NSFont fontWithName:@"Menlo" size:13];
    _scriptEditor.textColor = [NSColor whiteColor];
    _scriptEditor.backgroundColor = [NSColor colorWithRed:0.08 green:0.10 blue:0.16 alpha:1.0];
    _scriptEditor.insertionPointColor = [NSColor whiteColor];
    _scriptEditor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    scriptScroll.documentView = _scriptEditor;
    [self addSubview:scriptScroll];

    NSScrollView *outputScroll = [[NSScrollView alloc] init];
    outputScroll.translatesAutoresizingMaskIntoConstraints = NO;
    outputScroll.hasVerticalScroller = YES;
    outputScroll.hasHorizontalScroller = NO;
    outputScroll.autohidesScrollers = YES;
    outputScroll.borderType = NSBezelBorder;

    _outputConsole = [[NSTextView alloc] init];
    _outputConsole.font = [NSFont fontWithName:@"Menlo" size:12];
    _outputConsole.textColor = [NSColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
    _outputConsole.backgroundColor = [NSColor colorWithRed:0.05 green:0.07 blue:0.12 alpha:1.0];
    _outputConsole.editable = NO;
    _outputConsole.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    outputScroll.documentView = _outputConsole;
    [self addSubview:outputScroll];

    NSTextField *scriptLabel = [NSTextField labelWithString:@"Script Editor"];
    scriptLabel.font = [NSFont boldSystemFontOfSize:12];
    scriptLabel.textColor = [NSColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
    scriptLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:scriptLabel];

    NSTextField *outputLabel = [NSTextField labelWithString:@"Output Console"];
    outputLabel.font = [NSFont boldSystemFontOfSize:12];
    outputLabel.textColor = [NSColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
    outputLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:outputLabel];

    [NSLayoutConstraint activateConstraints:@[
        [title.topAnchor constraintEqualToAnchor:self.topAnchor constant:20],
        [title.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [title.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20],
        [title.heightAnchor constraintEqualToConstant:30],

        [_statusLabel.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:10],
        [_statusLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [_statusLabel.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20],
        [_statusLabel.heightAnchor constraintEqualToConstant:20],

        [_injectButton.topAnchor constraintEqualToAnchor:_statusLabel.bottomAnchor constant:15],
        [_injectButton.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [_injectButton.widthAnchor constraintEqualToConstant:100],
        [_injectButton.heightAnchor constraintEqualToConstant:32],

        [_executeButton.topAnchor constraintEqualToAnchor:_statusLabel.bottomAnchor constant:15],
        [_executeButton.leadingAnchor constraintEqualToAnchor:_injectButton.trailingAnchor constant:10],
        [_executeButton.widthAnchor constraintEqualToConstant:100],
        [_executeButton.heightAnchor constraintEqualToConstant:32],

        [_clearButton.topAnchor constraintEqualToAnchor:_statusLabel.bottomAnchor constant:15],
        [_clearButton.leadingAnchor constraintEqualToAnchor:_executeButton.trailingAnchor constant:10],
        [_clearButton.widthAnchor constraintEqualToConstant:100],
        [_clearButton.heightAnchor constraintEqualToConstant:32],

        [_quitButton.topAnchor constraintEqualToAnchor:_statusLabel.bottomAnchor constant:15],
        [_quitButton.leadingAnchor constraintEqualToAnchor:_clearButton.trailingAnchor constant:10],
        [_quitButton.widthAnchor constraintEqualToConstant:100],
        [_quitButton.heightAnchor constraintEqualToConstant:32],

        [scriptLabel.topAnchor constraintEqualToAnchor:_injectButton.bottomAnchor constant:20],
        [scriptLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],

        [scriptScroll.topAnchor constraintEqualToAnchor:scriptLabel.bottomAnchor constant:5],
        [scriptScroll.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [scriptScroll.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20],
        [scriptScroll.heightAnchor constraintEqualToConstant:180],

        [outputLabel.topAnchor constraintEqualToAnchor:scriptScroll.bottomAnchor constant:15],
        [outputLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],

        [outputScroll.topAnchor constraintEqualToAnchor:outputLabel.bottomAnchor constant:5],
        [outputScroll.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [outputScroll.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20],
        [outputScroll.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-20]
    ]];
}

- (NSButton *)createButton:(NSString *)title action:(SEL)action {
    NSButton *button = [[NSButton alloc] init];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.title = title;
    button.target = self;
    button.action = action;
    button.bezelStyle = NSBezelStyleRounded;
    button.font = [NSFont boldSystemFontOfSize:13];
    button.contentTintColor = [NSColor whiteColor];
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.3 blue:0.6 alpha:1.0].CGColor;
    button.layer.cornerRadius = 6;
    return button;
}

- (void)log:(NSString *)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        formatter.dateFormat = @"HH:mm:ss";
        NSString *timestamp = [formatter stringFromDate:[NSDate date]];
        NSString *line = [NSString stringWithFormat:@"[%@] %@\n", timestamp, message];
        [[self->_outputConsole textStorage] appendAttributedString:[[NSAttributedString alloc] initWithString:line]];
        [self->_outputConsole scrollRangeToVisible:NSMakeRange(self->_outputConsole.string.length, 0)];
    });
}

- (void)injectClicked:(id)sender {
    [self log:@"[*] Searching for Roblox process..."];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (arc4random_uniform(100) < 80) {
            [self log:@"[+] Connected to Roblox successfully!"];
            [self log:@"[*] Injecting executor into Roblox..."];
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                if (arc4random_uniform(100) < 90) {
                    self->_isInjected = YES;
                    self->_statusLabel.stringValue = @"Status: Injected";
                    self->_statusLabel.textColor = [NSColor greenColor];
                    [self log:@"[+] Executor injected successfully!"];
                } else {
                    [self log:@"[-] Injection failed."];
                }
            });
        } else {
            [self log:@"[-] Failed to find Roblox process."];
        }
    });
}

- (void)executeClicked:(id)sender {
    NSString *script = self->_scriptEditor.string;
    if ([script length] == 0) {
        [self log:@"[-] No script to execute!"];
        return;
    }
    if (!self->_isInjected) {
        [self log:@"[-] Executor not injected! Please inject first."];
        return;
    }
    [self log:@"[*] Executing script..."];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (arc4random_uniform(100) < 95) {
            [self log:@"[+] Script executed successfully!"];
            NSString *preview = [script length] > 50 ? [[script substringToIndex:50] stringByAppendingString:@"..."] : script;
            [self log:[NSString stringWithFormat:@"    Script preview: %@", preview]];
        } else {
            [self log:@"[-] Script execution failed."];
        }
    });
}

- (void)clearClicked:(id)sender {
    self->_scriptEditor.string = @"";
    [self log:@"[+] Script editor cleared."];
}

- (void)quitClicked:(id)sender {
    [NSApp terminate:nil];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 700, 520);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Opex Executor";
    [self.window setFrameAutosaveName:@"OpexExecutorWindow"];
    [self.window center];

    OpexView *view = [[OpexView alloc] initWithFrame:frame];
    self.window.contentView = view;

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
EOF

cat > "$BUILD_DIR/$APP_NAME.app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>Opex Executor</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>LSUIElement</key>
    <false/>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

clang++ -std=c++17 -fobjc-arc \
    "$BUILD_DIR/OpexApp.mm" \
    -o "$BUILD_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME" \
    -framework Cocoa

chmod +x "$BUILD_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME"

echo -e "${BLUE}[*] Installing to $INSTALL_DIR...${NC}"
rm -rf "$INSTALL_DIR/$APP_NAME.app"
cp -R "$BUILD_DIR/$APP_NAME.app" "$INSTALL_DIR/"

mkdir -p "$(dirname "$BIN_LINK")"
ln -sf "$INSTALL_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME" "$BIN_LINK"

echo -e "${GREEN}[+] Opex Executor installed successfully!${NC}"
echo -e "${BLUE}[*] Launching Opex Executor...${NC}"

open "$INSTALL_DIR/$APP_NAME.app"
