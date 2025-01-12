#include "TunDevice.hpp"
#include "Logger.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <iostream>
#include <algorithm>

using namespace std;

TunDevice::TunDevice(const string &name, bool isServer)
    : name_(name), isServer_(isServer), fd_(-1)
{
}

TunDevice::~TunDevice()
{
    if (fd_ >= 0)
        close(fd_);
}

bool TunDevice::initialize()
{
    // Open TUN device with read/write permissions
    if ((fd_ = open("/dev/net/tun", O_RDWR)) < 0)
    {
        perror("Failed to open /dev/net/tun");
        return false;
    }

    // Create and initialize interface request structure
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    // Set flags for TUN device (IFF_TUN: TUN device, IFF_NO_PI: No packet info)
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    // Copy interface name to the request structure
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ);

    // Configure TUN device using ioctl system call
    if (ioctl(fd_, TUNSETIFF, (void *)&ifr) < 0)
    {
        perror("ioctl(TUNSETIFF)");
        close(fd_);
        fd_ = -1;
        return false;
    }

    // Set IP address based on server/client role
    string ip = isServer_ ? "10.0.0.1/24" : "10.0.1.2/24";
    return configureInterface(ip);
}

bool TunDevice::configureInterface(const string &ip)
{
    // Define network ranges for server and client
    string serverNet = "10.0.0.0/24"; // Server network range
    string clientNet = "10.0.1.0/24"; // Client network range
    string ipAddress;
    string routeNet;

    if (isServer_)
    {
        ipAddress = "10.0.0.1/24"; // Server IP
        routeNet = clientNet;      // Route to client network
    }
    else
    {
        ipAddress = "10.0.1.2/24"; // Client IP
        routeNet = serverNet;      // Route to server network
    }

    // Set IP address for the interface
    string cmd = "ip addr add " + ipAddress + " dev " + name_;
    cout << "Executing: " << cmd << endl;
    if (system(cmd.c_str()) != 0)
    {
        perror("Failed to set IP address");
        return false;
    }

    // Bring up the network interface
    cmd = "ip link set dev " + name_ + " up";
    cout << "Executing: " << cmd << endl;
    if (system(cmd.c_str()) != 0)
    {
        perror("Failed to bring up interface");
        return false;
    }

    // Add route to the opposite network
    cmd = "ip route add " + routeNet + " dev " + name_;
    cout << "Executing: " << cmd << endl;
    if (system(cmd.c_str()) != 0)
    {
        perror("Failed to add route");
        return false;
    }

    // Clean up any incorrect routes (silently ignore errors)
    // if (isServer_) {
    //     cmd = "ip route del " + serverNet + " dev " + name_ + " 2>/dev/null || true";
    //     system(cmd.c_str());
    // } else {
    //     cmd = "ip route del " + clientNet + " dev " + name_ + " 2>/dev/null || true";
    //     system(cmd.c_str());
    // }

    return true;
}

ssize_t TunDevice::read(char *buffer, size_t len)
{
    // Read data from TUN device into buffer
    ssize_t n = ::read(fd_, buffer, len);
    if (n > 0)
    {
        // Debug output: Show first 16 bytes of packet
        printf("Packet: ");
        for (int i = 0; i < min(n, (ssize_t)16); i++)
        {
            printf("%02x ", (unsigned char)buffer[i]);
        }
        printf("\n");

        // Extract and display IP version from packet header
        if (n >= 1)
        {
            unsigned char version = (buffer[0] >> 4) & 0xF;
            printf("IP Version: %d\n", version);
        }
    }
    return n;
}
// Write data to the TUN device
ssize_t TunDevice::write(const char *buffer, size_t len)
{
    ssize_t total = 0;
    ssize_t n;

    while (total < len)
    {
        n = ::write(fd_, buffer + total, len - total);
        if (n < 0)
        {
            // Continue if interrupted or would block
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        total += n;
    }
    return total;
}
