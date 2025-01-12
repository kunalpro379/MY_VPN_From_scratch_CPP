# Firewall Configuration Documentation

## iptables Chains

```
                    ┌─────────────┐
                    │   Input    │
                    └─────────────┘
                          ↑
 ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
 │  Incoming   │ → │  Forward   │ → │  Outgoing   │
 └─────────────┘   └─────────────┘   └─────────────┘
```

## Basic Rules Structure

```bash
# Default policies
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# Allow established connections
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Allow VPN traffic
iptables -A INPUT -i tun+ -j ACCEPT
```

## VPN-Specific Rules

```bash
# Allow VPN port
iptables -A INPUT -p udp --dport 1194 -j ACCEPT

# Forward VPN traffic
iptables -A FORWARD -i tun0 -o eth0 -j ACCEPT
iptables -A FORWARD -i eth0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT
```

## Security Zones

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Trusted    │ →  │    VPN     │ →  │ Untrusted   │
│  (Local)    │    │  (tun0)    │    │ (Internet)  │
└─────────────┘    └─────────────┘    └─────────────┘
```

## Rule Management

```bash
# List rules
iptables -L -v -n

# Save rules
iptables-save > /etc/iptables/rules.v4

# Restore rules
iptables-restore < /etc/iptables/rules.v4
