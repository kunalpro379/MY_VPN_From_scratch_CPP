# TUN/TAP Devices Documentation

## Overview

TUN (network TUNnel) and TAP (network TAP) are virtual network kernel drivers:
- TUN: Operates at Layer 3 (IP)
- TAP: Operates at Layer 2 (Ethernet)

## TUN vs TAP Comparison

```
+----------------+-------------------------+-------------------------+
| Feature        | TUN                    | TAP                    |
+----------------+-------------------------+-------------------------+
| OSI Layer      | Layer 3 (Network)      | Layer 2 (Data Link)    |
| Handles        | IP Packets             | Ethernet Frames        |
| Use Case       | Routing/VPN            | Network Bridging       |
| Header Size    | IP Header              | Ethernet Header        |
| Frame Type     | IP Packets Only        | All Ethernet Types     |
+----------------+-------------------------+-------------------------+
```

## TUN Device Implementation

```cpp
// Basic TUN device creation
int create_tun(char *dev) {
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);
    
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    
    ioctl(fd, TUNSETIFF, &ifr);
    return fd;
}
```

## Packet Flow

```
Application Layer
      ↑↓
    TUN/TAP
      ↑↓
 Network Stack
```

## Configuration Commands

```bash
# Create TUN interface
ip tuntap add dev tun0 mode tun

# Configure IP
ip addr add 10.8.0.1/24 dev tun0

# Activate interface
ip link set dev tun0 up
