#include "../tun_interface/TunDevice.hpp"
#include "../tun_interface/VPNConnection.hpp"
#include <iostream>
using namespace std;

class VPNClient {
private:
    // TUN device object
    TunDevice tun;
    // VPN connection object
    VPNConnection vpn;
    // Interface name
    string interfaceName;
    // Server IP address
    string serverIP;
    // Port number
    int port;
    // Buffer for data transfer
    char buffer[TunDevice::BUFFER_SIZE];
    // Counters for packets sent and received
    unsigned long packets_sent, packets_received;

public:
    // Constructor
    VPNClient(const string& iface, const string& serverIP, int port)
        : tun(iface, false), vpn(false), interfaceName(iface), 
          serverIP(serverIP), port(port), packets_sent(0), packets_received(0) {}

    // Initialize the VPN client
    bool initialize() {
        // Initialize the TUN device
        if (!tun.initialize()) {
            cerr << "Failed to initialize TUN device\n";
            return false;
        }
        cout << "Successfully initialized TUN device " << interfaceName << endl;

        // Connect to the VPN server
        if (!vpn.connect(serverIP, port)) {
            cerr << "Failed to connect to server\n";
            return false;
        }
        cout << "Connected to server " << serverIP << endl;
        return true;
    }

    // Run the VPN client
    bool run() {
        while (true) {
            // File descriptor set for select
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(tun.getFd(), &readSet);
            FD_SET(vpn.getFd(), &readSet);

            // Get the maximum file descriptor
            int maxFd = max(tun.getFd(), vpn.getFd()) + 1;
            // Wait for data on either TUN or VPN
            if (select(maxFd, &readSet, NULL, NULL, NULL) < 0) {
                perror("select()");
                printStatistics();
                return false;
            }

            // Handle data from TUN to VPN
            if (FD_ISSET(tun.getFd(), &readSet)) {
                if (!handleTunToVPN()) {
                    printStatistics();
                    return false;
                }
            }

            // Handle data from VPN to TUN
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
    // Handle data transfer from TUN to VPN
    bool handleTunToVPN() {
        // Read data from TUN device
        int len = tun.read(buffer, sizeof(buffer));
        if (len <= 0) return false;
        
        // Increment packets sent counter
        packets_sent++;
        cout << "TUN -> NET [" << packets_sent << "]: " 
                    << len << " bytes" << endl;
        
        // Write packet length to VPN
        uint16_t plength = htons(len);
        if (vpn.write(reinterpret_cast<char*>(&plength), sizeof(plength)) <= 0 ||
            vpn.write(buffer, len) <= 0) {
            cerr << "Failed to write to network\n";
            return false;
        }
        return true;
    }

    // Handle data transfer from VPN to TUN
    bool handleVPNToTun() {
        // Read packet length from VPN
        uint16_t plength;
        if (vpn.read(reinterpret_cast<char*>(&plength), sizeof(plength)) <= 0) {
            cerr << "Connection closed by peer\n";
            return false;
        }

        // Convert packet length to host byte order
        int len = ntohs(plength);
        if (len > TunDevice::BUFFER_SIZE) {
            cerr << "Packet too large: " << len << endl;
            return false;
        }

        // Read packet data from VPN
        if (vpn.read(buffer, len) <= 0) {
            cerr << "Failed to read packet data\n";
            return false;
        }

        // Increment packets received counter
        packets_received++;
        cout << "NET -> TUN [" << packets_received << "]: " 
                    << len << " bytes" << endl;

        // Write data to TUN device
        if (tun.write(buffer, len) <= 0) {
            cerr << "Failed to write to TUN\n";
            return false;
        }
        return true;
    }

    // Print statistics of packets sent and received
    void printStatistics() {
        cout << "\nStatistics:\n"
                  << "Packets sent: " << packets_sent << "\n"
                  << "Packets received: " << packets_received << endl;
    }
};
