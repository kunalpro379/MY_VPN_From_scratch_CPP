# NAT and Routing Documentation

## Network Address Translation (NAT)

### Types of NAT
```
1. Source NAT (SNAT)
   [Local] --> [NAT Device] --> [Internet]
   
2. Destination NAT (DNAT)
   [Internet] --> [NAT Device] --> [Local]
   
3. Masquerade
   [Dynamic IP] --> [NAT Device] --> [Internet]
```

### NAT Configuration

```bash
# Basic NAT masquerade
iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE

# SNAT with fixed IP
iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 203.0.113.1

# Port forwarding (DNAT)
iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 80 -j DNAT --to 10.8.0.2:80
```

## Routing

### Routing Table Structure
```
Destination     Gateway         Genmask         Flags   Interface
0.0.0.0         10.8.0.1       0.0.0.0         UG      tun0
10.8.0.0        0.0.0.0        255.255.255.0   U       tun0
192.168.1.0     0.0.0.0        255.255.255.0   U       eth0
```

### Routing Commands
```bash
# Add route
ip route add 10.8.0.0/24 dev tun0

# Delete route
ip route del default

# Change default gateway
ip route replace default via 10.8.0.1 dev tun0
```
