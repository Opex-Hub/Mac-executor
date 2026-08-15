// Opex.cpp
// Compile: clang++ -std=c++17 -framework Cocoa -framework Foundation -x objective-c++ Opex.cpp -o OpexExecutor

#import <Cocoa/Cocoa.h>
#include <iostream>
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

    // Check if injection succeeded by looking for common error strings
    if (output.find("error:") != std::string::npos ||
        output.find("failed") != std::string::npos ||
        output.find("task_for_pid") != std::string::npos) {
        return false;
    }
    return true;
}

// ======================================================================
// GUI Application Delegate
// ======================================================================
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) NSTextView *logTextView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self setupUI];
}

- (void)setupUI {
    // Create window
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 300)
                                              styleMask:(NSWindowStyleMaskTitled |
                                                         NSWindowStyleMaskClosable |
                                                         NSWindowStyleMaskMiniaturizable |
                                                         NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Opex Executor"];
    [self.window center];

    // Dark vibrancy background
    NSVisualEffectView *vibrancy = [[NSVisualEffectView alloc] initWithFrame:self.window.contentView.bounds];
    vibrancy.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    vibrancy.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    vibrancy.state = NSVisualEffectStateActive;
    vibrancy.material = NSVisualEffectMaterialDark;
    [self.window.contentView addSubview:vibrancy];

    NSView *contentView = self.window.contentView;
    CGFloat margin = 20;
    CGFloat y = margin;

    // Inject button (centered)
    NSButton *injectButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin, y, 120, 40)];
    [injectButton setTitle:@"Inject"];
    [injectButton setTarget:self];
    [injectButton setAction:@selector(injectDylib:)];
    [injectButton setBezelStyle:NSBezelStyleRounded];
    [injectButton setFont:[NSFont systemFontOfSize:16 weight:NSFontWeightSemibold]];
    [contentView addSubview:injectButton];
    y += 50;

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
            // Convert output to NSString and append
            NSString *outputStr = [NSString stringWithUTF8String:output.c_str()];
            if (outputStr.length > 0) {
                [self appendLog:outputStr];
            }
            if (success) {
                [self appendLog:@"Injection successful."];
            } else {
                [self appendLog:@"Injection failed. Check above for details."];
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
