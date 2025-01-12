#include <iostream>
#include <string.h>
#include <unistd.h>

struct VPNConfig {
    char ifaceName[100];
    char serverIP[100];
    int port;
    bool isServer;
};

bool parseArguments(int argc, char* argv[], VPNConfig& config) {
    int opt;
    // Initialize default values
    config.port = 55555;
    config.isServer = false;
    config.ifaceName[0] = '\0';
    config.serverIP[0] = '\0';

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "i:sc:p:")) != -1) {
        switch (opt) {
            case 'i': strcpy(config.ifaceName, optarg); break;
            case 's': config.isServer = true; break;
            case 'c': strcpy(config.serverIP, optarg); config.isServer = false; break;
            case 'p': config.port = atoi(optarg); break;
            default: return false;
        }
    }

    return validateConfig(config);
}

bool validateConfig(const VPNConfig& config) {
    if (strlen(config.ifaceName) == 0) {
        std::cerr << "Interface name is required (-i option)\n";
        return false;
    }

    if (!config.isServer && strlen(config.serverIP) == 0) {
        std::cerr << "Server IP is required for client mode (-c option)\n";
        return false;
    }

    return true;
}

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " -i <interface> [-s|-c <server_ip>] [-p <port>]\n";
}
