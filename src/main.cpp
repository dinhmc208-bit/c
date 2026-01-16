#include "config.h"
#include "files.h"
#include "cli.h"
#include "scan_engine.h"
#include "brute_engine.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    (void)signal;  // Unused in simple handler
    g_running = false;
    std::cout << "\n\n[SIGNAL] Stopping...\n\n";
}

int main() {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize components
    FilesHandler files;
    files.deployFolders();
    files.deployFiles();
    
    Config config;
    if (!config.load(files.getConfigPath())) {
        config.setDefault();
        config.save(files.getConfigPath());
    }
    
    ScanEngine scan_engine;
    BruteEngine brute_engine;
    
    // Start CLI
    CLI cli(config, scan_engine, brute_engine);
    cli.run();
    
    // Stop engines on exit
    scan_engine.stop();
    brute_engine.stop();
    
    // Save config on exit
    if (config.auto_save) {
        config.save(files.getConfigPath());
    }
    
    return 0;
}

