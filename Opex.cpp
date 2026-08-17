#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ctime>

#ifdef __APPLE__
#include <cstdlib>
#endif

class RobloxInjector {
private:
    bool isConnected;
    bool isExecutorInjected;
    std::vector<std::string> executedScripts;

public:
    RobloxInjector() : isConnected(false), isExecutorInjected(false) {}

    bool connectToRoblox() {
        if (isConnected) return true;
        
        setTerminalColor(34); // Blue
        std::cout << "[*] Searching for Roblox process..." << std::endl;
        resetTerminalColor();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        
        // Simulate process detection (80% success rate)
        if (rand() % 100 < 80) {
            isConnected = true;
            setTerminalColor(32); // Green
            std::cout << "[+] Connected to Roblox successfully!" << std::endl;
            resetTerminalColor();
            return true;
        } else {
            setTerminalColor(31); // Red
            std::cout << "[-] Failed to find Roblox process." << std::endl;
            resetTerminalColor();
            return false;
        }
    }

    bool injectExecutor() {
        if (!connectToRoblox()) return false;
        if (isExecutorInjected) return true;

        setTerminalColor(34); // Blue
        std::cout << "[*] Injecting executor into Roblox..." << std::endl;
        resetTerminalColor();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // Simulate injection (90% success rate)
        if (rand() % 100 < 90) {
            isExecutorInjected = true;
            setTerminalColor(32); // Green
            std::cout << "[+] Executor injected successfully!" << std::endl;
            resetTerminalColor();
            return true;
        } else {
            setTerminalColor(31); // Red
            std::cout << "[-] Injection failed." << std::endl;
            resetTerminalColor();
            return false;
        }
    }

    bool executeScript(const std::string& script) {
        if (!isExecutorInjected) {
            setTerminalColor(31); // Red
            std::cout << "[-] Executor not injected! Please inject first." << std::endl;
            resetTerminalColor();
            return false;
        }

        setTerminalColor(34); // Blue
        std::cout << "[*] Executing script..." << std::endl;
        resetTerminalColor();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Simulate script execution (95% success rate)
        if (rand() % 100 < 95) {
            executedScripts.push_back(script);
            setTerminalColor(32); // Green
            std::cout << "[+] Script executed successfully!" << std::endl;
            resetTerminalColor();
            
            // Show a preview of the executed script (first 50 chars)
            std::string preview = script.substr(0, std::min(50, (int)script.length()));
            if (script.length() > 50) preview += "...";
            setTerminalColor(34); // Blue
            std::cout << "    Script preview: " << preview << std::endl;
            resetTerminalColor();
            
            return true;
        } else {
            setTerminalColor(31); // Red
            std::cout << "[-] Script execution failed." << std::endl;
            resetTerminalColor();
            return false;
        }
    }

    void clearScripts() {
        executedScripts.clear();
        setTerminalColor(32); // Green
        std::cout << "[+] Script history cleared!" << std::endl;
        resetTerminalColor();
    }

    void disconnect() {
        if (isConnected) {
            setTerminalColor(34); // Blue
            std::cout << "[*] Disconnecting from Roblox..." << std::endl;
            resetTerminalColor();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            isConnected = false;
            isExecutorInjected = false;
            setTerminalColor(32); // Green
            std::cout << "[+] Disconnected successfully!" << std::endl;
            resetTerminalColor();
        }
    }

    bool isReadyToExecute() const {
        return isExecutorInjected;
    }

    bool getInjectionStatus() const {
        return isExecutorInjected;
    }

    void setTerminalColor(int colorCode) {
        #ifdef __APPLE__
        std::cout << "\033[" << colorCode << "m";
        #endif
    }

    void resetTerminalColor() {
        #ifdef __APPLE__
        std::cout << "\033[0m";
        #endif
    }
};

void clearScreen() {
    #ifdef __APPLE__
    system("clear");
    #endif
}

void drawUI(RobloxInjector& injector, const std::string& scriptInput) {
    clearScreen();
    
    // Set blue color for UI elements
    injector.setTerminalColor(34);
    
    // Draw top border
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                          OPEX EXECUTOR                       ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    // Draw injection status
    std::cout << "║ Injection Status: ";
    if (injector.getInjectionStatus()) {
        injector.setTerminalColor(32); // Green
        std::cout << "INJECTED                                     ║";
    } else {
        injector.setTerminalColor(31); // Red
        std::cout << "NOT INJECTED                                  ║";
    }
    injector.setTerminalColor(34); // Blue
    std::cout << std::endl;
    
    // Draw control buttons
    std::cout << "║ [I] Inject     [E] Execute     [C] Clear     [Q] Quit        ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    // Draw script input area
    std::cout << "║                     SCRIPT EDITOR                            ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    // Display script content
    std::istringstream iss(scriptInput);
    std::string line;
    int lineCount = 0;
    while (std::getline(iss, line) && lineCount < 10) {
        std::cout << "║ " << line;
        // Pad with spaces to fill the line
        int padding = 60 - line.length() - 2;
        for (int i = 0; i < padding; i++) std::cout << " ";
        std::cout << "║" << std::endl;
        lineCount++;
    }
    
    // Fill remaining lines if needed
    while (lineCount < 10) {
        std::cout << "║                                                              ║" << std::endl;
        lineCount++;
    }
    
    // Bottom border
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    // Input prompt
    std::cout << "Enter script (type 'END' to finish) or command: ";
    injector.resetTerminalColor();
}

int main() {
    RobloxInjector injector;
    std::string scriptInput = "";
    std::string line;
    
    srand(time(0)); // Initialize random seed
    
    while (true) {
        drawUI(injector, scriptInput);
        
        std::getline(std::cin, line);
        
        // Handle commands
        if (line == "I" || line == "i") {
            injector.injectExecutor();
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
        else if (line == "E" || line == "e") {
            if (!scriptInput.empty()) {
                injector.executeScript(scriptInput);
            } else {
                injector.setTerminalColor(31); // Red
                std::cout << "No script to execute!" << std::endl;
                injector.resetTerminalColor();
            }
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
        else if (line == "C" || line == "c") {
            scriptInput = "";
            injector.clearScripts();
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
        else if (line == "Q" || line == "q") {
            injector.disconnect();
            break;
        }
        else if (line == "END") {
            // Do nothing, we're already processing input
        }
        else {
            // Add line to script unless it's a command
            if (!line.empty()) {
                if (!scriptInput.empty()) {
                    scriptInput += "\n";
                }
                scriptInput += line;
            }
        }
    }
    
    return 0;
}
