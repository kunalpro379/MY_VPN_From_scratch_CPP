#!/bin/bash

# Exit on error
set -e

# Restore original routing
if [ -f /tmp/vpn_original_gateway ]; then
    ORIGINAL_GATEWAY=$(cat /tmp/vpn_original_gateway)
    sudo ip route del default
    sudo ip route add default via $ORIGINAL_GATEWAY
    rm /tmp/vpn_original_gateway
fi

# Clean up tun interface
sudo ip link set dev tun0 down

# Remove NAT rules if on server
sudo iptables -t nat -D POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE 2>/dev/null
sudo iptables -D FORWARD -i tun0 -o eth0 -j ACCEPT 2>/dev/null
sudo iptables -D FORWARD -i eth0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null

# Restore DNS settings
if [ -f /tmp/vpn_dns_backup ]; then
    sudo cp /tmp/vpn_dns_backup /etc/resolv.conf
    rm /tmp/vpn_dns_backup
fi
