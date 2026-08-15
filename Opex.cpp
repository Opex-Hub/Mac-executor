// opex_executor.mm
// Compile: clang++ -std=c++17 -framework Cocoa -framework Foundation -o opex_executor opex_executor.mm
// Run: ./opex_executor

#import <Cocoa/Cocoa.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

// ======================================================================
// Embedded Lua module (Open Cloud execution) – same as provided
// ======================================================================
static const std::string LUA_MODULE = R"LUA(
--!optimize 2
local process = require("@lune/process")
local net = require("@lune/net")
local serde = require("@lune/serde")

local API_KEY_ENV_VAR = "RBX_API_KEY"
local MAX_PAYLOAD_SIZE = 4_000_000
local MAX_INPUT_SIZE = 104_857_600
local MAX_EXECUTION_TIME = 300
local MAX_QUERY_YIELD = 16
local BASE_URL = "https://apis.roblox.com/cloud/v2"
local TASK_CREATE_LATEST = `{BASE_URL}/universes/%s/places/%s/luau-execution-session-tasks`
local BINARY_INPUT_CREATE = `{BASE_URL}/universes/%s/luau-execution-session-task-binary-inputs`
local USER_AGENT = `Dekkonot/Lune-OpenCloud-Execution 1.0.0; {_VERSION}`
local RATE_LIMIT_HEADER = "x-ratelimit-reset"
local ID_PATTERN = "([%w%-]+)/tasks/([%w%-]+)$"

local api_key_override: string?

local function api_key(): string
    if api_key_override then
        return api_key_override
    else
        local api_key = process.env[API_KEY_ENV_VAR]
        if not api_key then
            error(`no Open Cloud API key specified`)
        else
            return api_key
        end
    end
end

export type LuauTaskState = "STATE_UNSPECIFIED" | "QUEUED" | "PROCESSING" | "CANCELLED" | "COMPLETE" | "FAILED"
export type LuauExecutionTask = {
    path: string, user: string, state: LuauTaskState, timeout: string,
    binaryInput: string, enableBinaryOutput: boolean, binaryOutputUri: string,
    createTime: string?, updateTime: string?, output: { results: { unknown } }?,
    error: TaskError?, script: string?,
}
export type MessageType = "MESSAGE_TYPE_UNSPECIFIED" | "OUTPUT" | "INFO" | "WARNING" | "ERROR"
export type StructuredLog = { message: string, createTime: string, messageType: MessageType }
type LuauExecutionSessionTaskLogs = {
    luauExecutionSessionTaskLogs: { { path: string, messages: { string }, structuredMessages: { StructuredLog } } },
    nextPageToken: string,
}
export type TaskErrorType = "ERROR_CODE_UNSPECIFIED" | "SCRIPT_ERROR" | "DEADLINE_EXCEEDED" | "OUTPUT_SIZE_LIMIT_EXCEEDED" | "INTERNAL_ERROR"
export type TaskError = { code: TaskErrorType, message: string }
type InputUploadResponse = { path: string, size: number, uploadUri: string }

function upload_input(input: buffer | string, universe_id: string): string
    local input_size = if typeof(input) == "buffer" then buffer.len(input) else #input
    if input_size > MAX_INPUT_SIZE then
        error(`input too large`)
    end
    local request = { size = input_size }
    local result = net.request({
        method = "POST",
        url = string.format(BINARY_INPUT_CREATE, universe_id),
        body = serde.encode("json", request),
        headers = {
            ["User-Agent"] = USER_AGENT,
            ["Content-Type"] = "application/json",
            ["X-API-Key"] = api_key(),
        },
    })
    if not result.ok then
        error(`failed to upload input: {result.statusCode} {result.statusMessage}`)
    end
    local response: InputUploadResponse = serde.decode("json", result.body)

    local result_2 = net.request({
        method = "PUT",
        url = response.uploadUri,
        body = input,
        headers = {
            ["User-Agent"] = USER_AGENT,
            ["Content-Type"] = "application/octet-stream",
            ["X-API-Key"] = api_key(),
        },
    })
    if not result_2.ok then
        error(`failed to upload input: {result_2.statusCode} {result_2.statusMessage}`)
    end
    return response.path
end

local function make_task(url: string, script: string, timeout: number?, enable_binary_output: boolean?, binary_input: string?): LuauExecutionTask
    local request = {
        script = script,
        timeout = `{timeout}s`,
        enableBinaryOutput = enable_binary_output or false,
        binaryInput = binary_input,
    }
    local body = serde.encode("json", request)
    if #body > MAX_PAYLOAD_SIZE then
        error("request too large")
    end
    local result = net.request({
        method = "POST",
        url = url,
        body = body,
        headers = {
            ["User-Agent"] = USER_AGENT,
            ["Content-Type"] = "application/json",
            ["X-API-Key"] = api_key(),
        },
    })
    if not result.ok then
        error(`failed to create task: {result.statusCode} {result.statusMessage}`)
    end
    return serde.decode("json", result.body)
end

function get_logs(task: LuauExecutionTask, view: "VIEW_UNSPECIFIED" | "FLAT" | "STRUCTURED"): LuauExecutionSessionTaskLogs
    local result = net.request({
        method = "GET",
        url = `{BASE_URL}/{task.path}/logs`,
        headers = { ["User-Agent"] = USER_AGENT, ["X-API-Key"] = api_key() },
        query = { ["view"] = view },
    })
    if not result.ok then
        error(`failed to get logs: {result.statusCode}`)
    end
    return serde.decode("json", result.body)
end

function query_status(task: LuauExecutionTask): LuauExecutionTask
    local result = net.request({
        method = "GET",
        url = `{BASE_URL}/{task.path}`,
        headers = { ["User-Agent"] = USER_AGENT, ["X-API-Key"] = api_key() },
    })
    if not result.ok then
        error(`failed to check status: {result.statusCode}`)
    end
    return serde.decode("json", result.body)
end

local luau_execute = {}

function luau_execute.create_task_latest(universe_id: string, place_id: string, script: buffer | string, timeout: number?, enable_binary_output: boolean?, binary_input: (buffer | string)?): LuauExecutionTask
    timeout = timeout or MAX_EXECUTION_TIME
    if timeout < 1 or timeout > MAX_EXECUTION_TIME then
        error(`timeout out of range`)
    end
    if type(script) == "buffer" then script = buffer.tostring(script) end
    local input_path
    if binary_input then input_path = upload_input(binary_input, universe_id) end
    return make_task(string.format(TASK_CREATE_LATEST, universe_id, place_id), script :: string, timeout :: number, enable_binary_output, input_path)
end

function luau_execute.check_status(task: LuauExecutionTask): LuauTaskState
    return query_status(task).state
end

function luau_execute.await_finish(task: LuauExecutionTask, timeout: number?): boolean
    timeout = timeout or MAX_EXECUTION_TIME
    local task_lib = require("@lune/task")
    local now = os.clock()
    local yield = 0.1
    local status = luau_execute.check_status(task)
    if status ~= "PROCESSING" then return status == "COMPLETE" end
    repeat
        task_lib.wait(yield)
        yield = math.min(yield * 2, MAX_QUERY_YIELD)
        status = luau_execute.check_status(task)
    until status ~= "PROCESSING" or os.clock() - now >= timeout
    return status == "COMPLETE"
end

function luau_execute.get_flat_logs(task: LuauExecutionTask): { string }
    local logs = get_logs(task, "FLAT")
    return logs.luauExecutionSessionTaskLogs[1].messages
end

function luau_execute.get_structured_logs(task: LuauExecutionTask): { StructuredLog }
    local logs = get_logs(task, "STRUCTURED")
    return logs.luauExecutionSessionTaskLogs[1].structuredMessages
end

function luau_execute.get_output(task: LuauExecutionTask): { unknown }
    local status = query_status(task)
    if not status.output then return {} else return status.output.results end
end

function luau_execute.get_error(task: LuauExecutionTask): TaskError?
    local status = query_status(task)
    if status.state ~= "PROCESSING" then
        return status.error
    else
        luau_execute.await_finish(task)
        status = query_status(task)
        return status.error
    end
end

function luau_execute.session_id(task: LuauExecutionTask): string
    local _, session_id = string.match(task.path, ID_PATTERN)
    return session_id
end

function luau_execute.task_id(task: LuauExecutionTask): string
    local task_id, _ = string.match(task.path, ID_PATTERN)
    return task_id
end

function luau_execute.set_api_key(api_key: (buffer | string)?)
    if type(api_key) == "buffer" then api_key_override = buffer.tostring(api_key)
    elseif type(api_key) == "string" then api_key_override = api_key
    else api_key_override = nil end
end

return luau_execute
)LUA";

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
// macOS Injection routine using Mach APIs
// ======================================================================
static bool injectDylib(const std::string& dylibPath) {
    // Find Roblox PID
    std::string pidStr = execCommand("pgrep -f Roblox");
    if (pidStr.empty()) {
        std::cerr << "Roblox is not running." << std::endl;
        return false;
    }
    pid_t pid = std::stoi(pidStr);
    std::cout << "Found Roblox PID: " << pid << std::endl;

    // Get task port
    task_t task;
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
    if (kr != KERN_SUCCESS) {
        std::cerr << "task_for_pid failed: " << mach_error_string(kr) << std::endl;
        std::cerr << "Run with sudo or disable SIP." << std::endl;
        return false;
    }

    // Allocate memory for the dylib path string
    mach_vm_address_t remoteString = 0;
    mach_vm_size_t stringSize = dylibPath.size() + 1;
    kr = mach_vm_allocate(task, &remoteString, stringSize, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        std::cerr << "mach_vm_allocate failed: " << mach_error_string(kr) << std::endl;
        return false;
    }

    // Write the dylib path into the remote process
    kr = mach_vm_write(task, remoteString, (vm_offset_t)dylibPath.c_str(), (mach_msg_type_number_t)stringSize);
    if (kr != KERN_SUCCESS) {
        std::cerr << "mach_vm_write failed: " << mach_error_string(kr) << std::endl;
        return false;
    }

    // Find dlopen address in our own process (same in target if same architecture)
    void* dlopenAddr = dlsym(RTLD_DEFAULT, "dlopen");
    if (!dlopenAddr) {
        std::cerr << "Failed to find dlopen." << std::endl;
        return false;
    }

    // Create a remote thread to call dlopen(path, RTLD_NOW)
    // Arguments: dlopen, path, RTLD_NOW
    mach_vm_address_t remoteStack = 0;
    mach_vm_size_t stackSize = 64 * 1024; // 64KB stack
    kr = mach_vm_allocate(task, &remoteStack, stackSize, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        std::cerr << "Failed to allocate remote stack." << std::endl;
        return false;
    }

    // Prepare arguments on remote stack (x86_64/arm64 ABI)
    // We'll use a simple approach: use thread_create_running with a small stub
    // that calls dlopen with the given arguments.
    // For brevity, we'll skip the stub and directly use thread_create_running
    // with a function pointer that matches dlopen's signature.
    // This is not perfectly ABI-compliant but works on macOS in many cases.
    // A more robust method would use a remote code stub.

    // For simplicity, we'll use lldb as a fallback (as in previous Python script)
    std::cout << "Using lldb injection (fallback)..." << std::endl;
    std::string lldbCmd = "lldb -b -o 'process attach --pid " + std::to_string(pid) +
                          "' -o 'expr (void*)dlopen(\"" + dylibPath + "\", 2)' -o 'detach' -o 'quit'";
    int ret = system(lldbCmd.c_str());
    if (ret != 0) {
        std::cerr << "lldb injection failed." << std::endl;
        return false;
    }
    return true;
}

// ======================================================================
// GUI Application Delegate
// ======================================================================
@interface AppDelegate : NSObject <NSApplicationDelegate, NSTextFieldDelegate>
@property (strong) NSWindow *window;
@property (strong) NSTextView *luaTextView;
@property (strong) NSTextField *apiKeyField;
@property (strong) NSTextField *universeIdField;
@property (strong) NSTextField *placeIdField;
@property (strong) NSTextField *dylibPathField;
@property (strong) NSTextView *logTextView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self setupUI];
}

- (void)setupUI {
    // Create window
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 700, 600)
                                              styleMask:(NSWindowStyleMaskTitled |
                                                         NSWindowStyleMaskClosable |
                                                         NSWindowStyleMaskMiniaturizable |
                                                         NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Opex Executor"];
    [self.window center];

    // Enable vibrancy / modern look
    NSVisualEffectView *vibrancy = [[NSVisualEffectView alloc] initWithFrame:self.window.contentView.bounds];
    vibrancy.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    vibrancy.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    vibrancy.state = NSVisualEffectStateActive;
    vibrancy.material = NSVisualEffectMaterialDark;
    [self.window.contentView addSubview:vibrancy];

    // Main content view
    NSView *contentView = self.window.contentView;
    CGFloat margin = 20;
    CGFloat y = margin;

    // API Key label & field
    NSTextField *apiLabel = [NSTextField labelWithString:@"API Key:"];
    apiLabel.frame = NSMakeRect(margin, y, 80, 24);
    apiLabel.textColor = [NSColor whiteColor];
    [contentView addSubview:apiLabel];

    self.apiKeyField = [[NSTextField alloc] initWithFrame:NSMakeRect(margin + 90, y, 250, 24)];
    self.apiKeyField.placeholderString = @"Enter Roblox Open Cloud API Key";
    self.apiKeyField.textColor = [NSColor whiteColor];
    self.apiKeyField.backgroundColor = [NSColor colorWithWhite:0.2 alpha:0.8];
    self.apiKeyField.bordered = YES;
    self.apiKeyField.bezeled = YES;
    self.apiKeyField.bezelStyle = NSTextFieldRoundedBezel;
    [contentView addSubview:self.apiKeyField];
    y += 30;

    // Universe ID
    NSTextField *universeLabel = [NSTextField labelWithString:@"Universe ID:"];
    universeLabel.frame = NSMakeRect(margin, y, 90, 24);
    universeLabel.textColor = [NSColor whiteColor];
    [contentView addSubview:universeLabel];

    self.universeIdField = [[NSTextField alloc] initWithFrame:NSMakeRect(margin + 100, y, 150, 24)];
    self.universeIdField.placeholderString = @"Universe ID";
    self.universeIdField.textColor = [NSColor whiteColor];
    self.universeIdField.backgroundColor = [NSColor colorWithWhite:0.2 alpha:0.8];
    [contentView addSubview:self.universeIdField];
    y += 30;

    // Place ID
    NSTextField *placeLabel = [NSTextField labelWithString:@"Place ID:"];
    placeLabel.frame = NSMakeRect(margin, y, 80, 24);
    placeLabel.textColor = [NSColor whiteColor];
    [contentView addSubview:placeLabel];

    self.placeIdField = [[NSTextField alloc] initWithFrame:NSMakeRect(margin + 90, y, 150, 24)];
    self.placeIdField.placeholderString = @"Place ID";
    self.placeIdField.textColor = [NSColor whiteColor];
    self.placeIdField.backgroundColor = [NSColor colorWithWhite:0.2 alpha:0.8];
    [contentView addSubview:self.placeIdField];
    y += 30;

    // Dylib path
    NSTextField *dylibLabel = [NSTextField labelWithString:@"Dylib Path:"];
    dylibLabel.frame = NSMakeRect(margin, y, 90, 24);
    dylibLabel.textColor = [NSColor whiteColor];
    [contentView addSubview:dylibLabel];

    self.dylibPathField = [[NSTextField alloc] initWithFrame:NSMakeRect(margin + 100, y, 250, 24)];
    self.dylibPathField.placeholderString = @"/path/to/executor.dylib";
    self.dylibPathField.textColor = [NSColor whiteColor];
    self.dylibPathField.backgroundColor = [NSColor colorWithWhite:0.2 alpha:0.8];
    [contentView addSubview:self.dylibPathField];

    NSButton *browseButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin + 360, y, 80, 24)];
    [browseButton setTitle:@"Browse..."];
    [browseButton setTarget:self];
    [browseButton setAction:@selector(browseDylib:)];
    [contentView addSubview:browseButton];
    y += 35;

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
    NSButton *executeButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin, y, 100, 30)];
    [executeButton setTitle:@"Execute"];
    [executeButton setTarget:self];
    [executeButton setAction:@selector(executeLua:)];
    [executeButton setBezelStyle:NSBezelStyleRounded];
    [executeButton setButtonType:NSButtonTypeMomentaryPushIn];
    [contentView addSubview:executeButton];

    NSButton *clearButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin + 110, y, 100, 30)];
    [clearButton setTitle:@"Clear"];
    [clearButton setTarget:self];
    [clearButton setAction:@selector(clearLua:)];
    [contentView addSubview:clearButton];

    NSButton *injectButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin + 220, y, 100, 30)];
    [injectButton setTitle:@"Inject"];
    [injectButton setTarget:self];
    [injectButton setAction:@selector(injectDylib:)];
    [contentView addSubview:injectButton];
    y += 40;

    // Log area
    NSScrollView *logScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(margin, y, self.window.contentView.bounds.size.width - 2*margin, 100)];
    logScroll.hasVerticalScroller = YES;
    logScroll.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:logScroll];

    NSTextView *logView = [[NSTextView alloc] initWithFrame:logScroll.bounds];
    logView.autoresizingMask = NSViewWidthSizable;
    logView.font = [NSFont fontWithName:@"Menlo" size:10];
    logView.textColor = [NSColor whiteColor];
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

- (void)browseDylib:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    if ([panel runModal] == NSModalResponseOK) {
        NSURL *url = panel.URLs.firstObject;
        self.dylibPathField.stringValue = url.path;
    }
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
    NSString *apiKey = self.apiKeyField.stringValue;
    NSString *universeId = self.universeIdField.stringValue;
    NSString *placeId = self.placeIdField.stringValue;

    if (apiKey.length == 0 || universeId.length == 0 || placeId.length == 0) {
        [self appendLog:@"Error: Please fill in API Key, Universe ID, and Place ID."];
        return;
    }

    // Write Lua module to temp file
    NSString *tempDir = NSTemporaryDirectory();
    NSString *modulePath = [tempDir stringByAppendingPathComponent:@"opex_lua_module.luau"];
    NSString *scriptPath = [tempDir stringByAppendingPathComponent:@"opex_script.luau"];

    NSError *error = nil;
    [[NSString stringWithUTF8String:LUA_MODULE.c_str()] writeToFile:modulePath atomically:YES encoding:NSUTF8StringEncoding error:&error];
    if (error) {
        [self appendLog:[NSString stringWithFormat:@"Error writing Lua module: %@", error.localizedDescription]];
        return;
    }

    // Write wrapper script that sets env and runs module
    NSString *wrapper = [NSString stringWithFormat:
        @"local process = require(\"@lune/process\")\n"
        @"process.env.RBX_API_KEY = \"%@\"\n"
        @"local execute = require(\"%@\")\n"
        @"local task = execute.create_task_latest(\"%@\", \"%@\", [[%@]])\n"
        @"execute.await_finish(task)\n"
        @"local logs = execute.get_flat_logs(task)\n"
        @"for _, log in ipairs(logs) do print(log) end\n"
        @"local output = execute.get_output(task)\n"
        @"for _, v in ipairs(output) do print(v) end\n"
        @"local err = execute.get_error(task)\n"
        @"if err then print(\"ERROR: \" .. err.message) end\n",
        apiKey, modulePath, universeId, placeId, luaCode];
    [wrapper writeToFile:scriptPath atomically:YES encoding:NSUTF8StringEncoding error:&error];
    if (error) {
        [self appendLog:[NSString stringWithFormat:@"Error writing wrapper script: %@", error.localizedDescription]];
        return;
    }

    // Run with lune
    [self appendLog:@"Executing Lua script via Open Cloud..."];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        std::string cmd = "lune run " + std::string([scriptPath UTF8String]) + " 2>&1";
        std::string output = execCommand(cmd);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self appendLog:[NSString stringWithUTF8String:output.c_str()]];
        });
    });
}

- (void)injectDylib:(id)sender {
    NSString *dylibPath = self.dylibPathField.stringValue;
    if (dylibPath.length == 0) {
        [self appendLog:@"Error: Please specify a dylib path."];
        return;
    }
    [self appendLog:@"Injecting dylib..."];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        bool success = injectDylib([dylibPath UTF8String]);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (success) {
                [self appendLog:@"Injection successful."];
            } else {
                [self appendLog:@"Injection failed. Check logs."];
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
