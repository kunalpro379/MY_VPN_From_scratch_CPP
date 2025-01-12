#include "../server/VPNServer.cpp"
#include "../client/VPNClient.cpp"
#include "utils.cpp"
#include <iostream>
#include <unistd.h>
using namespace std;

int main(int argc, char* argv[]) {
    VPNConfig config;

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (!parseArguments(argc, argv, config)) {
        printUsage(argv[0]);
        return 1;
    }

    try {
        if (config.isServer) {
            cout << "Starting VPN server on interface " << config.ifaceName 
                 << " port " << config.port << endl;
            VPNServer server(config.ifaceName, config.port);
            if (!server.initialize()) {
                cerr << "Failed to initialize server\n";
                return 1;
            }
            server.run();
        } else {
            cout << "Connecting to " << config.serverIP << ":" << config.port 
                 << " via " << config.ifaceName << endl;
            VPNClient client(config.ifaceName, config.serverIP, config.port);
            if (!client.initialize()) {
                cerr << "Failed to initialize client\n";
                return 1;
            }
            
            int retries = 3;
            while (retries-- > 0) {
                if (client.run()) break;
                if (retries > 0) {
                    cout << "Retrying connection... (" << retries << " attempts left)\n";
                    sleep(2);
                }
            }
        }
    } catch (const std::exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
