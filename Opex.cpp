// Opex.cpp
// Compile: clang++ -std=c++17 -framework Cocoa -framework Foundation -x objective-c++ Opex.cpp -o OpexExecutor

#import <Cocoa/Cocoa.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstdio>

// ======================================================================
// Helper: execute a command and capture output
// ======================================================================
static std::string execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// ======================================================================
// Injection routine (uses lldb) – returns output string
// ======================================================================
static bool injectDylib(const std::string& dylibPath, std::string& output) {
    // Check if Roblox is running
    std::string pidStr = execCommand("pgrep -f Roblox");
    if (pidStr.empty()) {
        output = "Roblox is not running.";
        return false;
    }
    pid_t pid = std::stoi(pidStr);
    output += "Found Roblox PID: " + std::to_string(pid) + "\n";

    // Inject using lldb and capture its output
    std::string lldbCmd = "lldb -b -o 'process attach --pid " + std::to_string(pid) +
                          "' -o 'expr (void*)dlopen(\"" + dylibPath + "\", 2)' -o 'detach' -o 'quit' 2>&1";
    output += execCommand(lldbCmd);

    if (output.find("error:") != std::string::npos ||
        output.find("failed") != std::string::npos ||
        output.find("task_for_pid") != std::string::npos) {
        return false;
    }
    return true;
}

// ======================================================================
// Execute Lua via injected dylib (calls execute_lua function)
// ======================================================================
static bool executeLuaInjected(const std::string& luaCode, std::string& output) {
    std::string pidStr = execCommand("pgrep -f Roblox");
    if (pidStr.empty()) {
        output = "Roblox is not running (inject first).";
        return false;
    }
    pid_t pid = std::stoi(pidStr);

    // Escape Lua code for embedding in C string
    std::string escaped;
    for (char c : luaCode) {
        if (c == '\\') escaped += "\\\\";
        else if (c == '"') escaped += "\\\"";
        else if (c == '\n') escaped += "\\n";
        else escaped += c;
    }

    // Build lldb command to call execute_lua(escaped_code)
    std::string lldbCmd = "lldb -b -o 'process attach --pid " + std::to_string(pid) +
                          "' -o 'expr (void)execute_lua(\"" + escaped + "\")' -o 'detach' -o 'quit' 2>&1";
    output += execCommand(lldbCmd);

    if (output.find("error:") != std::string::npos ||
        output.find("failed") != std::string::npos) {
        return false;
    }
    return true;
}

// ======================================================================
// GUI Application Delegate
// ======================================================================
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) NSTextView *luaTextView;
@property (strong) NSTextView *logTextView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self setupUI];
}

- (void)setupUI {
    // Create window
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 650, 550)
                                              styleMask:(NSWindowStyleMaskTitled |
                                                         NSWindowStyleMaskClosable |
                                                         NSWindowStyleMaskMiniaturizable |
                                                         NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Opex Executor"];
    [self.window center];

    // Blue background
    NSVisualEffectView *vibrancy = [[NSVisualEffectView alloc] initWithFrame:self.window.contentView.bounds];
    vibrancy.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    vibrancy.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    vibrancy.state = NSVisualEffectStateActive;
    vibrancy.material = NSVisualEffectMaterialDark;
    [self.window.contentView addSubview:vibrancy];

    NSView *blueOverlay = [[NSView alloc] initWithFrame:self.window.contentView.bounds];
    blueOverlay.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    blueOverlay.wantsLayer = YES;
    blueOverlay.layer.backgroundColor = [[NSColor colorWithRed:0.1 green:0.2 blue:0.5 alpha:0.7] CGColor];
    [self.window.contentView addSubview:blueOverlay];

    NSView *contentView = self.window.contentView;
    CGFloat margin = 20;
    CGFloat y = margin;

    // Lua code text view (rich textbox)
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(margin, y, self.window.contentView.bounds.size.width - 2*margin, 250)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:scrollView];

    NSTextView *textView = [[NSTextView alloc] initWithFrame:scrollView.bounds];
    textView.autoresizingMask = NSViewWidthSizable;
    textView.font = [NSFont fontWithName:@"Menlo" size:12];
    textView.textColor = [NSColor whiteColor];
    textView.backgroundColor = [NSColor colorWithWhite:0.1 alpha:0.9];
    textView.richText = YES;
    textView.automaticQuoteSubstitutionEnabled = NO;
    scrollView.documentView = textView;
    self.luaTextView = textView;
    y += 260;

    // Buttons row
    NSButton *injectButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin, y, 100, 30)];
    [injectButton setTitle:@"Inject"];
    [injectButton setTarget:self];
    [injectButton setAction:@selector(injectDylib:)];
    [injectButton setBezelStyle:NSBezelStyleRounded];
    [injectButton setButtonType:NSButtonTypeMomentaryPushIn];
    [contentView addSubview:injectButton];

    NSButton *executeButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin + 110, y, 100, 30)];
    [executeButton setTitle:@"Execute"];
    [executeButton setTarget:self];
    [executeButton setAction:@selector(executeLua:)];
    [executeButton setBezelStyle:NSBezelStyleRounded];
    [executeButton setButtonType:NSButtonTypeMomentaryPushIn];
    [contentView addSubview:executeButton];

    NSButton *clearButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin + 220, y, 100, 30)];
    [clearButton setTitle:@"Clear"];
    [clearButton setTarget:self];
    [clearButton setAction:@selector(clearLua:)];
    [clearButton setBezelStyle:NSBezelStyleRounded];
    [clearButton setButtonType:NSButtonTypeMomentaryPushIn];
    [contentView addSubview:clearButton];
    y += 40;

    // Log area (blue text)
    NSScrollView *logScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(margin, y, self.window.contentView.bounds.size.width - 2*margin, 150)];
    logScroll.hasVerticalScroller = YES;
    logScroll.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:logScroll];

    NSTextView *logView = [[NSTextView alloc] initWithFrame:logScroll.bounds];
    logView.autoresizingMask = NSViewWidthSizable;
    logView.font = [NSFont fontWithName:@"Menlo" size:12];
    logView.textColor = [NSColor blueColor];
    logView.backgroundColor = [NSColor colorWithWhite:0.1 alpha:0.9];
    logView.editable = NO;
    logScroll.documentView = logView;
    self.logTextView = logView;

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)appendLog:(NSString*)message {
    NSString *current = self.logTextView.string;
    NSString *newText = [current stringByAppendingFormat:@"%@\n", message];
    self.logTextView.string = newText;
    [self.logTextView scrollRangeToVisible:NSMakeRange(newText.length, 0)];
}

- (void)clearLua:(id)sender {
    self.luaTextView.string = @"";
}

- (void)executeLua:(id)sender {
    NSString *luaCode = self.luaTextView.string;
    if (luaCode.length == 0) {
        [self appendLog:@"Error: No Lua code entered."];
        return;
    }

    [self appendLog:@"Executing Lua code..."];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        std::string output;
        bool success = executeLuaInjected([luaCode UTF8String], output);
        dispatch_async(dispatch_get_main_queue(), ^{
            NSString *outputStr = [NSString stringWithUTF8String:output.c_str()];
            if (outputStr.length > 0) {
                [self appendLog:outputStr];
            }
            if (success) {
                [self appendLog:@"Lua executed."];
            } else {
                [self appendLog:@"Execute failed. Make sure the dylib exports execute_lua and is injected."];
            }
        });
    });
}

- (void)injectDylib:(id)sender {
    // Hardcoded dylib path: same directory as executable
    NSString *executablePath = [[NSBundle mainBundle] executablePath];
    NSString *executableDir = [executablePath stringByDeletingLastPathComponent];
    NSString *dylibPath = [executableDir stringByAppendingPathComponent:@"executor.dylib"];

    if (![[NSFileManager defaultManager] fileExistsAtPath:dylibPath]) {
        [self appendLog:@"Error: executor.dylib not found next to the executable."];
        return;
    }

    [self appendLog:@"Injecting..."];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        std::string output;
        bool success = injectDylib([dylibPath UTF8String], output);
        dispatch_async(dispatch_get_main_queue(), ^{
            NSString *outputStr = [NSString stringWithUTF8String:output.c_str()];
            if (outputStr.length > 0) {
                [self appendLog:outputStr];
            }
            if (success) {
                [self appendLog:@"Injection successful."];
            } else {
                [self appendLog:@"Injection failed. Check that Roblox is running and SIP is disabled."];
            }
        });
    });
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// ======================================================================
// main
// ======================================================================
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
