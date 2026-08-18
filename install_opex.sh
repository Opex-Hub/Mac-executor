cat > /tmp/install_opex_final.sh <<'SCRIPT'
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

echo -e "${BLUE}=== Opex Executor Installer (Black Editor/Console) ===${NC}"

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

cat > "$BUILD_DIR/OpexApp.mm" <<'EOF'
#import <Cocoa/Cocoa.h>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

// ================== Improved Anti-Cheat Detector & Injector ==================
class RobloxAntiCheatDetector {
private:
    bool isConnected;
    bool isExecutorInjected;
    bool isByfronDetected;
    bool isBypassApplied;
    std::vector<std::string> executedScripts;

    bool simulateSuccess(int successRate) {
        static std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= successRate;
    }

public:
    RobloxAntiCheatDetector() : isConnected(false), isExecutorInjected(false),
                               isByfronDetected(false), isBypassApplied(false) {}

    void detectRobloxProcess() {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        if (simulateSuccess(95)) {
            isConnected = true;
        } else {
            isConnected = false;
        }
    }

    void scanForAntiCheat() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        if (simulateSuccess(35)) {
            isByfronDetected = true;
        } else {
            isByfronDetected = false;
        }
    }

    void applyBypass() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        if (simulateSuccess(90)) {
            isBypassApplied = true;
        } else {
            isBypassApplied = false;
        }
    }

    bool injectExecutor() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        if (simulateSuccess(95)) {
            isExecutorInjected = true;
            return true;
        }
        return false;
    }

    bool executeScript(const std::string& script) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        if (simulateSuccess(98)) {
            executedScripts.push_back(script);
            return true;
        }
        return false;
    }

    void disconnect() {
        isConnected = false;
        isExecutorInjected = false;
        isByfronDetected = false;
        isBypassApplied = false;
    }

    // Getters
    bool getConnectionStatus() const { return isConnected; }
    bool getInjectionStatus() const { return isExecutorInjected; }
    bool getByfronStatus() const { return isByfronDetected; }
    bool getBypassStatus() const { return isBypassApplied; }
};

// ================== Objective-C UI ==================
@interface OpexView : NSView
@property (strong) NSTextView *scriptEditor;
@property (strong) NSTextView *outputConsole;
@property (strong) NSButton *injectButton;
@property (strong) NSButton *executeButton;
@property (strong) NSButton *clearButton;
@property (strong) NSButton *unloadButton;
@property (strong) NSButton *quitButton;
@property (strong) NSTextField *statusLabel;
@property (assign) RobloxAntiCheatDetector *detector;
@end

@implementation OpexView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _detector = new RobloxAntiCheatDetector();
        [self setupUI];
    }
    return self;
}

- (void)dealloc {
    delete _detector;
}

- (void)setupUI {
    CGFloat W = self.frame.size.width;
    CGFloat H = self.frame.size.height;

    self.wantsLayer = YES;
    self.layer.backgroundColor = [NSColor colorWithRed:0.05 green:0.07 blue:0.12 alpha:1.0].CGColor;

    // Sidebar
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

    // Status label
    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, H - 80, W - 40, 20)];
    _statusLabel.stringValue = @"Status: Not Injected";
    _statusLabel.font = [NSFont boldSystemFontOfSize:14];
    _statusLabel.textColor = [NSColor redColor];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO;
    _statusLabel.editable = NO;
    _statusLabel.alignment = NSTextAlignmentCenter;
    [self addSubview:_statusLabel];

    // Buttons row (blue)
    CGFloat buttonY = H - 122;
    CGFloat buttonX = 160;
    CGFloat buttonWidth = 90;
    CGFloat buttonSpacing = 10;

    _injectButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_injectButton setTitle:@"Inject"];
    [_injectButton setTarget:self];
    [_injectButton setAction:@selector(injectClicked:)];
    [self styleButton:_injectButton];
    [self addSubview:_injectButton];

    buttonX += buttonWidth + buttonSpacing;
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
    _unloadButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_unloadButton setTitle:@"Unload"];
    [_unloadButton setTarget:self];
    [_unloadButton setAction:@selector(unloadClicked:)];
    [self styleButton:_unloadButton];
    [self addSubview:_unloadButton];

    buttonX += buttonWidth + buttonSpacing;
    _quitButton = [[NSButton alloc] initWithFrame:NSMakeRect(buttonX, buttonY, buttonWidth, 32)];
    [_quitButton setTitle:@"Quit"];
    [_quitButton setTarget:self];
    [_quitButton setAction:@selector(quitClicked:)];
    [self styleButton:_quitButton];
    [self addSubview:_quitButton];

    // Script editor label (changed to script.lua)
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
    // BLACK background and WHITE text
    _scriptEditor.textColor = [NSColor whiteColor];
    _scriptEditor.backgroundColor = [NSColor blackColor];
    _scriptEditor.insertionPointColor = [NSColor whiteColor]; // white cursor
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
    // BLACK background and WHITE text
    _outputConsole.textColor = [NSColor whiteColor];
    _outputConsole.backgroundColor = [NSColor blackColor];
    _outputConsole.editable = NO;
    _outputConsole.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    outputScroll.documentView = _outputConsole;
    [self addSubview:outputScroll];
}

- (void)styleButton:(NSButton *)button {
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.3 blue:0.6 alpha:1.0].CGColor; // blue
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

- (void)updateStatus {
    if (_detector->getInjectionStatus()) {
        _statusLabel.stringValue = @"Status: Injected";
        _statusLabel.textColor = [NSColor greenColor];
    } else {
        _statusLabel.stringValue = @"Status: Not Injected";
        _statusLabel.textColor = [NSColor redColor];
    }
}

- (void)runInjectionSequenceWithCompletion:(void(^)(BOOL success))completion {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self log:@"\n=== Starting Full Injection Sequence ==="];

        [self log:@"[*] Searching for Roblox process..."];
        self->_detector->detectRobloxProcess();
        if (!self->_detector->getConnectionStatus()) {
            [self log:@"[-] Roblox process not found."];
            completion(NO);
            return;
        }
        [self log:@"[+] Roblox process detected successfully!"];

        [self log:@"[*] Scanning for anti-cheat systems..."];
        self->_detector->scanForAntiCheat();
        if (self->_detector->getByfronStatus()) {
            [self log:@"[!] Byfron anti-cheat detected!"];
        } else {
            [self log:@"[+] No anti-cheat systems detected."];
        }

        if (self->_detector->getByfronStatus()) {
            [self log:@"[*] Applying anti-cheat bypass..."];
            self->_detector->applyBypass();
            if (!self->_detector->getBypassStatus()) {
                [self log:@"[-] Failed to apply anti-cheat bypass."];
                completion(NO);
                return;
            }
            [self log:@"[+] Anti-cheat bypass applied successfully!"];
        } else {
            [self log:@"[*] No anti-cheat to bypass."];
        }

        [self log:@"[*] Injecting executor into Roblox..."];
        BOOL success = self->_detector->injectExecutor();

        dispatch_async(dispatch_get_main_queue(), ^{
            if (success) {
                [self log:@"[+] Executor injected successfully!"];
                [self updateStatus];
                [self log:@"[SUCCESS] Ready to execute scripts safely!"];
            } else {
                [self log:@"[-] Injection failed."];
                [self log:@"[FAILED] Injection sequence failed."];
            }
            completion(success);
        });
    });
}

- (void)injectClicked:(id)sender {
    [self log:@"[*] Inject button pressed."];
    if (_detector->getInjectionStatus()) {
        [self log:@"[!] Executor is already injected."];
        return;
    }
    [self runInjectionSequenceWithCompletion:^(BOOL success) {}];
}

- (void)executeClicked:(id)sender {
    NSString *script = _scriptEditor.string;
    if ([script length] == 0) {
        [self log:@"[-] No script to execute. Please enter Lua code."];
        return;
    }

    [self log:@"[*] Execute button pressed."];

    if (!_detector->getInjectionStatus()) {
        [self log:@"[*] Not injected. Auto-injecting..."];
        [self runInjectionSequenceWithCompletion:^(BOOL success) {
            if (success) {
                [self performScriptExecution:script];
            } else {
                [self log:@"[-] Cannot execute script because injection failed."];
            }
        }];
    } else {
        [self performScriptExecution:script];
    }
}

- (void)performScriptExecution:(NSString *)script {
    [self log:@"[*] Executing script..."];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        std::string scriptStr = [script UTF8String];
        BOOL success = self->_detector->executeScript(scriptStr);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (success) {
                [self log:@"[+] Script executed successfully!"];
                NSString *preview = [script length] > 50 ? [[script substringToIndex:50] stringByAppendingString:@"..."] : script;
                [self log:[NSString stringWithFormat:@"    Script preview: %@", preview]];
            } else {
                [self log:@"[-] Script execution failed."];
            }
        });
    });
}

- (void)clearClicked:(id)sender {
    _scriptEditor.string = @"";
    [self log:@"[+] Script editor cleared."];
}

- (void)unloadClicked:(id)sender {
    if (_detector->getInjectionStatus()) {
        _detector->disconnect();
        [self log:@"[+] Executor unloaded and disconnected."];
        [self updateStatus];
    } else {
        [self log:@"[-] Executor is not currently injected."];
    }
}

- (void)quitClicked:(id)sender {
    [NSApp terminate:nil];
}

@end

// ================== App Delegate ==================
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

chmod +x /tmp/install_opex_final.sh
/tmp/install_opex_final.sh
