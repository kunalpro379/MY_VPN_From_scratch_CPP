# Custom VPN Implementation

A lightweight VPN implementation using TUN interfaces and OpenSSL for secure tunneling.

## Architecture
![VPN Architecture](images/vpn.jpg)

## Prerequisites

- Linux operating syste
- Root privileges
- g++ compiler
- OpenSSL development libraries
- TUN/TAP kernel module


## Installation
```bash
# Install required packages
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

## Documentation

Comprehensive documentation is available in the following files:

- [VPN Working and Core Concepts](docs/VPN_WORKING.md)
- [Virtual Network Interfaces](docs/VIRTUAL_INTERFACES.md)
- [TUN/TAP Implementation](docs/TUN_TAP.md)
- [NAT and Routing Configuration](docs/NAT_ROUTING.md)
- [Firewall Setup](docs/FIREWALL.md)
- [Encryption and Security](docs/ENCRYPTION.md)


### Component Diagram
```
+------------------+         +-------------------+         +------------------+
|    VPN Client    |         |    VPN Server     |         |  Internet/Target |
|                  |         |                   |         |                  |
| +-------------+  |   SSL   | +-------------+   |         |                  |
| |TUN Interface|  | Tunnel  | |TUN Interface|   |         |                  |
| |  (tun0)     |<--------->| |  (tun0)     |   |         |                  |
| +-------------+  |         | +-------------+   |         |                  |
|     ^           |         |        ^          |         |                  |
|     |           |         |        |          |         |                  |
| +-------------+  |         | +-----------+    |         |                  |
| |   Routing   |  |         | |   NAT     |    |         |                  |
| |   Table     |  |         | |   Rules   |    |         |                  |
| +-------------+  |         | +-----------+    |         |                  |
|     ^           |         |        ^          |         |                  |
|     |           |         |        |          |         |                  |
| +-------------+  |         | +-----------+    |   Raw   |                  |
| | Encryption  |  |         | | Encryption|    | Traffic |                  |
| | (OpenSSL)   |  |         | | (OpenSSL) |<----------->|                  |
| +-------------+  |         | +-----------+    |         |                  |
+------------------+         +-------------------+         +------------------+


Key Components:
- TUN Interface: Virtual network device for packet capture
- SSL Tunnel: Encrypted communication channel
- NAT & Routing: Network address translation and traffic routing
- OpenSSL: Handles encryption and certificate management
```

### Network Flow

1. Client application sends traffic
2. Client TUN interface captures packets
3. OpenSSL encrypts data
4. SSL tunnel transfers encrypted data
5. Server decrypts and processes packets
6. NAT rules handle routing to internet
7. Return traffic follows reverse path

## Usage

### Running the Server

```bash
./run.sh server
```

This will:
1. Clean up existing TUN interfaces
2. Generate SSL certificates
3. Enable IP forwarding
4. Configure NAT
5. Start the VPN server on port 55555

### Running the Client

```bash
./run.sh client
```

This will:
1. Clean up existing TUN interfaces
2. Connect to the VPN server
3. Configure routing through the VPN

### Cleaning Up

```bash
./run.sh clean
```

## Network Commands Explained

### TUN Interface Setup
```bash
# Create and configure TUN interface
ip link set dev tun0 up                  # Activate interface
ip addr add 10.8.0.1/24 dev tun0        # Assign IP address
```

### Routing Configuration
```bash
# View routing table
ip route show

# Configure default route
ip route add default via 10.8.0.1 dev tun0
```

### NAT and Forwarding
```bash
# Enable IP forwarding
sysctl -w net.ipv4.ip_forward=1

# Configure NAT
iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE
iptables -A FORWARD -i tun0 -o eth0 -j ACCEPT
iptables -A FORWARD -i eth0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT
```

## Security Features

- OpenSSL encryption for tunnel security
- Certificate-based authentication
- Immutable DNS configuration
- Proper network isolation

## Troubleshooting

1. **TUN Module Issues**
   ```bash
   lsmod | grep tun    # Check if TUN module is loaded
   ```

2. **Network Configuration**
   ```bash
   ip link show tun0   # Check interface status
   ip addr show tun0   # Check IP configuration
   ```

3. **Connection Testing**
   ```bash
   ping 10.8.0.1      # Test VPN server connectivity
   ping 10.8.0.2      # Test VPN client connectivity
   ```

## Safety Measures

- Back up your network configuration before making changes
- Save iptables rules:
  ```bash
  sudo iptables-save > /etc/iptables/rules.v4
  ```
- Protect DNS configuration:
  ```bash
  sudo chattr +i /etc/resolv.conf
  ```

## Example Outputs


![VPN Server Output](images/1.png)
![image](https://github.com/user-attachments/assets/c7abc013-0638-4cf7-8979-3de14270da56)

![VPN Client Connection](images/2.png)


![Network Traffic](images/3.png)


![TUN Interface](images/4.png)


![Tunneling Demo](images/5.png)


