#!/bin/bash

# Enable IP forwarding
sudo sysctl -w net.ipv4.ip_forward=1

# Configure tun interface
sudo ip link set dev tun0 up
sudo ip addr add 10.8.0.1/24 dev tun0

# Setup NAT rules
sudo iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE
sudo iptables -A FORWARD -i tun0 -o eth0 -j ACCEPT
sudo iptables -A FORWARD -i eth0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT

# Create iptables directory if it doesn't exist
sudo mkdir -p /etc/iptables

# Save iptables rules
sudo iptables-save > /etc/iptables/rules.v4
