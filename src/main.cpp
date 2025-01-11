#include "../server/VPNServer.cpp"
#include "../client/VPNClient.cpp"
#include <iostream>
#include <unistd.h>
#include <string.h> // 
use namespace std;
int main(int argc, char* argv[]) {
    char ifaceName[100]; // Interface name
    char serverIP[100];  // Server IP address
    int port = 55555;    // Default port
    bool isServer = false; // Flag to check if it's a server
    int opt; // Option for getopt

    // Check if the number of arguments is less than 2
    if (argc < 2) {
    cerr << "Usage: " << argv[0] << " -i <interface> [-s|-c <server_ip>] [-p <port>]\n";
        return 1;
    }

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "i:sc:p:")) != -1) {
        switch (opt) {
            case 'i': strcpy(ifaceName, optarg); break; // Copy interface name
            case 's': isServer = true; break; // Set server flag
            case 'c': strcpy(serverIP, optarg); isServer = false; break; // Copy server IP and unset server flag
            case 'p': port = atoi(optarg); break; // Convert port to integer
            default: return 1; // Return error for invalid option
        }
    }

    // Check if interface name is empty
    if (strlen(ifaceName) == 0) {
    cerr << "Usage: " << argv[0] << " -i <interface> [-s|-c <server_ip>] [-p <port>]\n";
    cerr << "Interface name is required (-i option)\n";
        return 1;
    }

    // Check if server IP is empty in client mode
    if (!isServer && strlen(serverIP) == 0) {
    cerr << "Server IP is required for client mode (-c option)\n";
        return 1;
    }

    try {
        // If it's a server
        if (isServer) {
        cout << "Starting VPN server on interface " << ifaceName << " port " << port << std::endl;
            VPNServer server(ifaceName, port); // Create VPNServer object
            if (!server.initialize()) { // Initialize server
            cerr << "Failed to initialize server\n";
                return 1;
            }
            server.run(); // Run server
        } else {
            // If it's a client
        cout << "Connecting to " << serverIP << ":" << port << " via " << ifaceName << std::endl;
            VPNClient client(ifaceName, serverIP, port); // Create VPNClient object
            if (!client.initialize()) { // Initialize client
            cerr << "Failed to initialize client\n";
                return 1;
            }
            
            // Add retry logic
            int retries = 3; // Number of retries
            while (retries-- > 0) { // Retry loop
                if (client.run()) break; // Run client and break if successful
                if (retries > 0) {
                cout << "Retrying connection... (" << retries << " attempts left)\n";
                    sleep(2); // Wait for 2 seconds before retrying
                }
            }
        }
    } catch (const std::exception& e) {
    cerr << "Fatal error: " << e.what() << std::endl; // Catch and print exception
        return 1;
    }

    return 0; // Return success
}
