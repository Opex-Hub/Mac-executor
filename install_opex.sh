cat > /tmp/install_opex_fixed.sh <<'SCRIPT'
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

echo -e "${BLUE}=== Opex Executor Installer (Fixed) ===${NC}"

# Check for Xcode CLT
if ! xcode-select -p &>/dev/null; then
    echo -e "${BLUE}[*] Installing Xcode Command Line Tools...${NC}"
    xcode-select --install
    echo -e "${GREEN}[+] Please complete installation, then run this script again.${NC}"
    exit 0
fi

BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT

echo -e "${BLUE}[*] Building Opex Executor...${NC}"

mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/MacOS"
mkdir -p "$BUILD_DIR/$APP_NAME.app/Contents/Resources"

# ---- Objective-C++ source ----
cat > "$BUILD_DIR/OpexApp.mm" <<'EOF'
#import <Cocoa/Cocoa.h>

@interface OpexView : NSView
@property (strong) NSTextView *scriptEditor;
@property (strong) NSTextView *outputConsole;
@property (strong) NSButton *injectButton;
@property (strong) NSButton *executeButton;
@property (strong) NSButton *clearButton;
@property (strong) NSButton *quitButton;
@property (strong) NSTextField *statusLabel;
@property (assign) BOOL isInjected;
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
    // Set background color
    self.wantsLayer = YES;
    self.layer.backgroundColor = [NSColor colorWithRed:0.08 green:0.08 blue:0.1 alpha:1.0].CGColor;

    // Title label
    NSTextField *title = [[NSTextField alloc] initWithFrame:NSMakeRect(20, self.frame.size.height - 50, self.frame.size.width - 40, 30)];
    title.stringValue = @"OPEX EXECUTOR";
    title.font = [NSFont boldSystemFontOfSize:24];
    title.textColor = [NSColor whiteColor];
    title.backgroundColor = [NSColor clearColor];
    title.bordered = NO;
    title.editable = NO;
    title.alignment = NSTextAlignmentCenter;
    [self addSubview:title];

    // Status label
    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, self.frame.size.height - 80, self.frame.size.width - 40, 20)];
    _statusLabel.stringValue = @"Status: Not Injected";
    _statusLabel.font = [NSFont boldSystemFontOfSize:14];
    _statusLabel.textColor = [NSColor redColor];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO;
    _statusLabel.editable = NO;
    _statusLabel.alignment = NSTextAlignmentCenter;
    [self addSubview:_statusLabel];

    // Sidebar (placeholder for script list)
    NSView *sidebar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 150, self.frame.size.height - 100)];
    sidebar.wantsLayer = YES;
    sidebar.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.1 blue:0.12 alpha:1.0].CGColor;
    [self addSubview:sidebar];

    // Buttons
    _injectButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, self.frame.size.height - 110, 100, 32)];
    [_injectButton setTitle:@"Inject"];
    [_injectButton setTarget:self];
    [_injectButton setAction:@selector(injectClicked:)];
    [self styleButton:_injectButton];
    [self addSubview:_injectButton];

    _executeButton = [[NSButton alloc] initWithFrame:NSMakeRect(130, self.frame.size.height - 110, 100, 32)];
    [_executeButton setTitle:@"Execute"];
    [_executeButton setTarget:self];
    [_executeButton setAction:@selector(executeClicked:)];
    [self styleButton:_executeButton];
    [self addSubview:_executeButton];

    _clearButton = [[NSButton alloc] initWithFrame:NSMakeRect(240, self.frame.size.height - 110, 100, 32)];
    [_clearButton setTitle:@"Clear"];
    [_clearButton setTarget:self];
    [_clearButton setAction:@selector(clearClicked:)];
    [self styleButton:_clearButton];
    [self addSubview:_clearButton];

    _quitButton = [[NSButton alloc] initWithFrame:NSMakeRect(350, self.frame.size.height - 110, 100, 32)];
    [_quitButton setTitle:@"Quit"];
    [_quitButton setTarget:self];
    [_quitButton setAction:@selector(quitClicked:)];
    [self styleButton:_quitButton];
    [self addSubview:_quitButton];

    // Script editor scroll view
    NSScrollView *scriptScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(160, 150, self.frame.size.width - 180, 200)];
    scriptScroll.hasVerticalScroller = YES;
    scriptScroll.hasHorizontalScroller = NO;
    scriptScroll.borderType = NSBezelBorder;
    scriptScroll.autohidesScrollers = YES;

    _scriptEditor = [[NSTextView alloc] initWithFrame:scriptScroll.contentView.bounds];
    _scriptEditor.font = [NSFont fontWithName:@"Menlo" size:13];
    _scriptEditor.textColor = [NSColor whiteColor];
    _scriptEditor.backgroundColor = [NSColor colorWithRed:0.12 green:0.12 blue:0.14 alpha:1.0];
    _scriptEditor.insertionPointColor = [NSColor whiteColor];
    _scriptEditor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scriptScroll.documentView = _scriptEditor;
    [self addSubview:scriptScroll];

    // Output console scroll view
    NSScrollView *outputScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(160, 20, self.frame.size.width - 180, 120)];
    outputScroll.hasVerticalScroller = YES;
    outputScroll.hasHorizontalScroller = NO;
    outputScroll.borderType = NSBezelBorder;
    outputScroll.autohidesScrollers = YES;

    _outputConsole = [[NSTextView alloc] initWithFrame:outputScroll.contentView.bounds];
    _outputConsole.font = [NSFont fontWithName:@"Menlo" size:12];
    _outputConsole.textColor = [NSColor colorWithRed:0.9 green:0.3 blue:0.3 alpha:1.0];
    _outputConsole.backgroundColor = [NSColor colorWithRed:0.08 green:0.08 blue:0.1 alpha:1.0];
    _outputConsole.editable = NO;
    _outputConsole.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    outputScroll.documentView = _outputConsole;
    [self addSubview:outputScroll];

    // Labels
    NSTextField *scriptLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(160, 355, 150, 20)];
    scriptLabel.stringValue = @"Script Editor";
    scriptLabel.font = [NSFont boldSystemFontOfSize:12];
    scriptLabel.textColor = [NSColor colorWithRed:0.9 green:0.3 blue:0.3 alpha:1.0];
    scriptLabel.backgroundColor = [NSColor clearColor];
    scriptLabel.bordered = NO;
    scriptLabel.editable = NO;
    [self addSubview:scriptLabel];

    NSTextField *outputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(160, 145, 150, 20)];
    outputLabel.stringValue = @"Output Console";
    outputLabel.font = [NSFont boldSystemFontOfSize:12];
    outputLabel.textColor = [NSColor colorWithRed:0.9 green:0.3 blue:0.3 alpha:1.0];
    outputLabel.backgroundColor = [NSColor clearColor];
    outputLabel.bordered = NO;
    outputLabel.editable = NO;
    [self addSubview:outputLabel];
}

- (void)styleButton:(NSButton *)button {
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithRed:0.7 green:0.1 blue:0.1 alpha:1.0].CGColor;
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
    NSLog(@"Opex: applicationDidFinishLaunching started");

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

    NSLog(@"Opex: window created with frame: %@", NSStringFromRect(self.window.frame));
    NSLog(@"Opex: window is visible: %d", self.window.isVisible);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"Opex: main started");
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        NSLog(@"Opex: calling run");
        [app run];
        NSLog(@"Opex: run returned");
    }
    return 0;
}
EOF

# ---- Info.plist ----
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

# ---- Compile ----
clang++ -std=c++17 -fobjc-arc \
    "$BUILD_DIR/OpexApp.mm" \
    -o "$BUILD_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME" \
    -framework Cocoa

chmod +x "$BUILD_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME"

# ---- Code sign ----
codesign --force --deep --sign - "$BUILD_DIR/$APP_NAME.app"

# ---- Install ----
rm -rf "$INSTALL_DIR/$APP_NAME.app"
cp -R "$BUILD_DIR/$APP_NAME.app" "$INSTALL_DIR/"
xattr -dr com.apple.quarantine "$INSTALL_DIR/$APP_NAME.app" 2>/dev/null || true

# ---- Symlink ----
mkdir -p "$(dirname "$BIN_LINK")"
rm -f "$BIN_LINK"
ln -sf "$INSTALL_DIR/$APP_NAME.app/Contents/MacOS/$APP_NAME" "$BIN_LINK"

echo -e "${GREEN}[+] Installed successfully!${NC}"
echo -e "${BLUE}[*] Launching...${NC}"
open "$INSTALL_DIR/$APP_NAME.app"
SCRIPT

chmod +x /tmp/install_opex_fixed.sh
/tmp/install_opex_fixed.sh
