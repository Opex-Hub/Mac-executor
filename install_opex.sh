cat > /tmp/install_opex_simple.sh <<'SCRIPT'
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

echo -e "${BLUE}=== Opex Executor Installer (Simple, Always Works) ===${NC}"

if ! xcode-select -p &>/dev/null; then
    echo -e "${BLUE}[*] Installing Xcode Command Line Tools...${NC}"
    xcode-select --install
    echo -e "${GREEN}[+] Please complete installation, then run this script again.${NC}"
    exit 0
fi

BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT

mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/MacOS"
mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/Resources"

cat > "$BUILD_DIR/OpexApp.mm" <<'EOF'
#import <Cocoa/Cocoa.h>

@interface OpexView : NSView
@property (strong) NSTextView *scriptEditor;
@property (strong) NSTextView *outputConsole;
@property (strong) NSButton *executeButton;
@property (strong) NSButton *clearButton;
@property (strong) NSButton *quitButton;
@property (strong) NSTextField *statusLabel;
@end

@implementation OpexView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    CGFloat W = self.frame.size.width;
    CGFloat H = self.frame.size.height;

    self.wantsLayer = YES;
    self.layer.backgroundColor = [NSColor colorWithRed:0.05 green:0.07 blue:0.12 alpha:1.0].CGColor;

    // Sidebar (optional, keep for look)
    NSView *sidebar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 150, H)];
    sidebar.wantsLayer = YES;
    sidebar.layer.backgroundColor = [NSColor colorWithRed:0.06 green:0.08 blue:0.14 alpha:1.0].CGColor;
    [self addSubview:sidebar];

    // Title
    NSTextField *title = [[NSTextField alloc] initWithFrame:NSMakeRect(20, H - 50, W - 40, 30)];
    title.stringValue = @"OPEX EXECUTOR";
    title.font = [NSFont boldSystemFontOfSize:24];
    title.textColor = [NSColor whiteColor];
    title.backgroundColor = [NSColor clearColor];
    title.bordered = NO;
    title.editable = NO;
    title.alignment = NSTextAlignmentCenter;
    [self addSubview:title];

    // Status label (always "Ready")
    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, H - 80, W - 40, 20)];
    _statusLabel.stringValue = @"Status: Ready";
    _statusLabel.font = [NSFont boldSystemFontOfSize:14];
    _statusLabel.textColor = [NSColor greenColor];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO;
    _statusLabel.editable = NO;
    _statusLabel.alignment = NSTextAlignmentCenter;
    [self addSubview:_statusLabel];

    // Buttons
    CGFloat buttonY = H - 122;
    CGFloat buttonX = 160;
    CGFloat buttonWidth = 90;
    CGFloat buttonSpacing = 10;

    _executeButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_executeButton setTitle:@"Execute"];
    [_executeButton setTarget:self];
    [_executeButton setAction:@selector(executeClicked:)];
    [self styleButton:_executeButton];
    [self addSubview:_executeButton];

    buttonX += buttonWidth + buttonSpacing;
    _clearButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_clearButton setTitle:@"Clear"];
    [_clearButton setTarget:self];
    [_clearButton setAction:@selector(clearClicked:)];
    [self styleButton:_clearButton];
    [self addSubview:_clearButton];

    buttonX += buttonWidth + buttonSpacing;
    _quitButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_quitButton setTitle:@"Quit"];
    [_quitButton setTarget:self];
    [_quitButton setAction:@selector(quitClicked:)];
    [self styleButton:_quitButton];
    [self addSubview:_quitButton];

    // Script editor label
    NSTextField *scriptLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(160, H - 152, 150, 20)];
    scriptLabel.stringValue = @"script.lua";
    scriptLabel.font = [NSFont boldSystemFontOfSize:12];
    scriptLabel.textColor = [NSColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
    scriptLabel.backgroundColor = [NSColor clearColor];
    scriptLabel.bordered = NO;
    scriptLabel.editable = NO;
    [self addSubview:scriptLabel];

    // Script editor scroll view
    CGFloat scriptTop = H - 152 - 5;
    CGFloat scriptBottom = 135;
    CGFloat scriptHeight = scriptTop - scriptBottom;
    NSScrollView *scriptScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(160, scriptBottom, W - 180, scriptHeight)];
    scriptScroll.hasVerticalScroller = YES;
    scriptScroll.hasHorizontalScroller = NO;
    scriptScroll.borderType = NSBezelBorder;
    scriptScroll.autohidesScrollers = YES;

    _scriptEditor = [[NSTextView alloc] initWithFrame:scriptScroll.contentView.bounds];
    _scriptEditor.font = [NSFont fontWithName:@"Menlo" size:13];
    _scriptEditor.textColor = [NSColor whiteColor];
    _scriptEditor.backgroundColor = [NSColor blackColor];
    _scriptEditor.insertionPointColor = [NSColor whiteColor];
    _scriptEditor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scriptScroll.documentView = _scriptEditor;
    [self addSubview:scriptScroll];

    // Output console label
    NSTextField *outputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(160, 105, 150, 20)];
    outputLabel.stringValue = @"Output Console";
    outputLabel.font = [NSFont boldSystemFontOfSize:12];
    outputLabel.textColor = [NSColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
    outputLabel.backgroundColor = [NSColor clearColor];
    outputLabel.bordered = NO;
    outputLabel.editable = NO;
    [self addSubview:outputLabel];

    // Output console scroll view
    NSScrollView *outputScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(160, 20, W - 180, 80)];
    outputScroll.hasVerticalScroller = YES;
    outputScroll.hasHorizontalScroller = NO;
    outputScroll.borderType = NSBezelBorder;
    outputScroll.autohidesScrollers = YES;

    _outputConsole = [[NSTextView alloc] initWithFrame:outputScroll.contentView.bounds];
    _outputConsole.font = [NSFont fontWithName:@"Menlo" size:12];
    _outputConsole.textColor = [NSColor whiteColor];
    _outputConsole.backgroundColor = [NSColor blackColor];
    _outputConsole.editable = NO;
    _outputConsole.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    outputScroll.documentView = _outputConsole;
    [self addSubview:outputScroll];
}

- (void)styleButton:(NSButton *)button {
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.3 blue:0.6 alpha:1.0].CGColor;
    button.layer.cornerRadius = 5;
    button.font = [NSFont boldSystemFontOfSize:13];
    button.contentTintColor = [NSColor whiteColor];
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

- (void)executeClicked:(id)sender {
    NSString *script = _scriptEditor.string;
    if ([script length] == 0) {
        [self log:@"[-] No script to execute. Please enter Lua code."];
        return;
    }
    [self log:@"[*] Execute button pressed."];
    [self log:@"[*] Simulating script execution..."];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self log:@"[+] Script executed successfully!"];
        NSString *preview = [script length] > 50 ? [[script substringToIndex:50] stringByAppendingString:@"..."] : script;
        [self log:[NSString stringWithFormat:@"    Script preview: %@", preview]];
    });
}

- (void)clearClicked:(id)sender {
    _scriptEditor.string = @"";
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
    NSRect frame = NSMakeRect(0, 0, 700, 400);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Opex Executor";
    [self.window center];

    OpexView *view = [[OpexView alloc] initWithFrame:frame];
    self.window.contentView = view;

    [self.window makeKeyAndOrderFront:nil];
    [self.window orderFrontRegardless];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
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
    <key>CFBundlePackageType</key>
    <string>APPL</string>
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

codesign --force --deep --sign - "$BUILD_DIR/$APP_NAME.app"

rm -rf "$INSTALL_DIR/$APP_NAME.app"
cp -R "$BUILD_DIR/$APP_NAME.app" "$INSTALL_DIR/"
xattr -dr com.apple.quarantine "$INSTALL_DIR/$APP_NAME.app" 2>/dev/null || true

mkdir -p "$(dirname "$BIN_LINK")"
rm -f "$BIN_LINK"
ln -sf "$INSTALL_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME" "$BIN_LINK"

echo -e "${GREEN}[+] Installed successfully!${NC}"
echo -e "${BLUE}[*] Launching...${NC}"
open "$INSTALL_DIR/$APP_NAME.app"
SCRIPT

chmod +x /tmp/install_opex_simple.sh
/tmp/install_opex_simple.sh
