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


TunDevice::~TunDevice() {
    if (fd_ >= 0) close(fd_);
}

bool TunDevice::initialize() {
    // Open the TUN device file
    if ((fd_ = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Failed to open /dev/net/tun");
        return false;
    }

    struct ifreq ifr;
    // Clear the ifreq structure
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags for TUN device without packet information
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    // Copy the interface name to ifr_name
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ);

    // Set the TUN device interface
    if (ioctl(fd_, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd_);
        fd_ = -1;
        return false;
    }

    // Configure IP based on server/client
    string ip = isServer_ ? "10.0.0.1/24" : "10.0.0.2/24";
    if (!configureInterface(ip)) {
        return false;
    }

    return true;
}

bool TunDevice::configureInterface(const string& ip) {
    // Construct the command to add IP address to the interface
    string cmd = "ip addr add " + ip + " dev " + name_;
    // Execute the command
    if (system(cmd.c_str()) != 0) {
        perror("Failed to set IP address");
        return false;
    }

    // Construct the command to bring up the interface
    cmd = "ip link set dev " + name_ + " up";
    // Execute the command
    if (system(cmd.c_str()) != 0) {
        perror("Failed to bring up interface");
        return false;
    }

    return true;
}

ssize_t TunDevice::read(char* buffer, size_t len) {
    // Read data from the TUN device
    ssize_t n = ::read(fd_, buffer, len);
    if (n > 0) {
        // Print first few bytes of packet for debugging
        printf("Packet: ");
        for (int i = 0; i < min(n, (ssize_t)16); i++) {
            printf("%02x ", (unsigned char)buffer[i]);
        }
        printf("\n");

        // Check IP header version
        if (n >= 1) {
            unsigned char version = (buffer[0] >> 4) & 0xF;
            printf("IP Version: %d\n", version);
        }
    }
    return n;
}

ssize_t TunDevice::write(const char* buffer, size_t len) {
    ssize_t total = 0;
    ssize_t n;
    
    // Write data to the TUN device
    while (total < len) {
        n = ::write(fd_, buffer + total, len - total);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return -1;
        }
        total += n;
    }
    return total;
}
