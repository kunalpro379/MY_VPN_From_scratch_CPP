#!/bin/bash

# Check if tun0 interface exists
if ! ip link show tun0 >/dev/null 2>&1; then
    echo "Error: tun0 interface not found"
    exit 1
fi

# More thorough cleanup of existing tun0 configuration
echo "Cleaning up existing tun0 configuration..."
sudo ip route del default via 10.8.0.1 dev tun0 2>/dev/null
sudo ip addr flush dev tun0 2>/dev/null
sudo ip link set dev tun0 down 2>/dev/null
sleep 1  # Give system time to clean up

# Save current default gateway
ORIGINAL_GATEWAY=$(ip route show default | awk '/default/ {print $3}')
echo $ORIGINAL_GATEWAY > /tmp/vpn_original_gateway

# Configure tun interface with error checking
echo "Configuring tun0 interface..."
sudo ip link set dev tun0 up || { echo "Failed to bring up tun0"; exit 1; }
sudo ip addr add 10.8.0.2/24 dev tun0 || { echo "Failed to add IP to tun0"; exit 1; }

# Route all traffic through VPN
sudo ip route del default
sudo ip route add default via 10.8.0.1 dev tun0

# Store current DNS settings
if [ -f /etc/resolv.conf ]; then
    grep "nameserver" /etc/resolv.conf > /tmp/vpn_dns_backup
fi

# Update DNS servers (example: using Google DNS)
echo "nameserver 8.8.8.8" | sudo tee /etc/resolv.conf > /dev/null
echo "nameserver 8.8.4.4" | sudo tee -a /etc/resolv.conf > /dev/null
