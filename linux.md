# Display current routing table
ip route show

# Remove default route through tun1 interface
sudo ip route del default via 10.0.0.1 dev tun1

# Make resolv.conf immutable to prevent changes
sudo chattr +i /etc/resolv.conf

# Check if TUN module is loaded in kernel
lsmod | grep tun

# Save current iptables rules to a file
sudo iptables-save > /etc/iptables/rules.v4

# Configure TUN interface with IP address
sudo ip addr add 10.0.0.1/24 dev tun0

# Bring up TUN interface
sudo ip link set dev tun0 up

# Enable IP forwarding in kernel
sudo sysctl -w net.ipv4.ip_forward=1

# TUN Interface Configuration and Verification Commands
# Show TUN interface status
ip link show tun0

# Show TUN interface IP configuration
ip addr show tun0

# Activate TUN interface
sudo ip link set dev tun0 up

# Assign IP address to TUN interface
sudo ip addr add 10.8.0.1/24 dev tun0

# Display routing table
ip route show

# Add default route through TUN interface
sudo ip route add default via 10.8.0.1 dev tun0

# Enable IP forwarding
sudo sysctl -w net.ipv4.ip_forward=1

# NAT Configuration
# Enable NAT for VPN clients
sudo iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE

# Allow forwarding from VPN to internet
sudo iptables -A FORWARD -i tun0 -o eth0 -j ACCEPT

# Allow established connections from internet to VPN
sudo iptables -A FORWARD -i eth0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT

# Test connectivity
# Ping VPN server
ping 10.8.0.1

# Ping VPN client
ping 10.8.0.2