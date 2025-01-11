bro make a code in old cpp style and add comment explaining the next line of code

#include "../tun_interface/TunDevice.hpp"
#include "../tun_interface/VPNConnection.hpp"
#include <iostream>
#include <cstring>  // For memset
using namespace std;
class VPNServer {
private:
    TunDevice tun;  // TUN device object
    VPNConnection vpn;  // VPN connection object
    string interfaceName;  // Interface name
    int port;  // Port number
    char buffer[TunDevice::BUFFER_SIZE];  // Buffer for data
    unsigned long packets_sent, packets_received;  // Packet counters

public:
    VPNServer(const string& iface, int port) 
        : tun(iface, true), vpn(true), interfaceName(iface), port(port) {
        // Initialize packet counters
        packets_sent = 0;
        packets_received = 0;
    }

    bool initialize() {
        // Initialize the TUN device
        if (!tun.initialize()) {
            cerr << "Failed to initialize TUN device\n";
            return false;
        }
        cout << "Successfully initialized TUN device " << interfaceName << endl;

        // Bind the VPN server to the specified port
        if (!vpn.bind(port)) {
            cerr << "Failed to bind VPN server to port " << port << endl;
            return false;
        }
        cout << "Successfully bound to port " << port << endl;

        // Accept a client connection
        if (!vpn.accept()) {
            cerr << "Failed to accept client connection\n";
            return false;
        }
        
        cout << "Server initialization complete\n";
        return true;
    }

    bool run() {
        while (true) {
            fd_set readSet;  // File descriptor set for select
            FD_ZERO(&readSet);  // Clear the set
            FD_SET(tun.getFd(), &readSet);  // Add TUN device file descriptor to the set
            FD_SET(vpn.getFd(), &readSet);  // Add VPN connection file descriptor to the set

            // Get the maximum file descriptor value
            int maxFd = max(tun.getFd(), vpn.getFd()) + 1;
            // Wait for data on either file descriptor
            if (select(maxFd, &readSet, nullptr, nullptr, nullptr) < 0) {
                perror("select()");
                printStatistics();
                return false;
            }

            // Check if there is data to read from the TUN device
            if (FD_ISSET(tun.getFd(), &readSet)) {
                if (!handleTunToVPN()) {
                    printStatistics();
                    return false;
                }
            }

            // Check if there is data to read from the VPN connection
            if (FD_ISSET(vpn.getFd(), &readSet)) {
                if (!handleVPNToTun()) {
                    printStatistics();
                    return false;
                }
            }
        }
        return true;
    }

private:
    bool handleTunToVPN() {
        // Read data from the TUN device
        int len = tun.read(buffer, sizeof(buffer));
        if (len <= 0) return false;
        
        packets_sent++;  // Increment the packet sent counter
        cout << "TUN -> NET [" << packets_sent << "]: " 
                    << len << " bytes" << endl;
        
        // Convert the length to network byte order
        uint16_t plength = htons(len);
        // Write the length and data to the VPN connection
        if (vpn.write(reinterpret_cast<char*>(&plength), sizeof(plength)) <= 0 ||
            vpn.write(buffer, len) <= 0) {
            cerr << "Failed to write to network\n";
            return false;
        }
        return true;
    }

    bool handleVPNToTun() {
        uint16_t plength;
        // Read the length of the packet from the VPN connection
        if (vpn.read(reinterpret_cast<char*>(&plength), sizeof(plength)) <= 0) {
            cerr << "Connection closed by peer\n";
            return false;
        }

        // Convert the length to host byte order
        int len = ntohs(plength);
        if (len > TunDevice::BUFFER_SIZE) {
            cerr << "Packet too large: " << len << endl;
            return false;
        }

        // Read the packet data from the VPN connection
        if (vpn.read(buffer, len) <= 0) {
            cerr << "Failed to read packet data\n";
            return false;
        }

        packets_received++;  // Increment the packet received counter
        cout << "NET -> TUN [" << packets_received << "]: " 
                    << len << " bytes" << endl;

        // Write the data to the TUN device
        if (tun.write(buffer, len) <= 0) {
            cerr << "Failed to write to TUN\n";
            return false;
        }
        return true;
    }

    void printStatistics() {
        // Print the packet statistics
        cout << "\nStatistics:\n"
                  << "Packets sent: " << packets_sent << "\n"
                  << "Packets received: " << packets_received << endl;
    }
};
