#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color


print_status() {
    echo -e "${GREEN}[*]${NC} $1"
}

# Function to print error
print_error() {
    echo -e "${RED}[!]${NC} $1"
}

# Compile function
compile() {
    print_status "Compiling VPN..."
    
    # Get absolute paths
    ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
    
    g++ -o vpn \
        "${ROOT_DIR}/src/main.cpp" \
        "${ROOT_DIR}/tun_interface/VPNConnection.cpp" \
        "${ROOT_DIR}/tun_interface/TunDevice.cpp" \
        "${ROOT_DIR}/tunneling/Tunnel.cpp" \
        -std=c++17 -lssl -lcrypto \
        -I"${ROOT_DIR}" \
        -I"${ROOT_DIR}/tun_interface" \
        -I"${ROOT_DIR}/tunneling"
    
    if [ $? -ne 0 ]; then
        print_error "Compilation failed!"
        exit 1
    fi
    print_status "Compilation successful!"
}

# Clean function
clean() {
    print_status "Cleaning up..."
    rm -f vpn
}

# Cleanup function
cleanup_interface() {
    local iface=$1
    print_status "Cleaning up interface $iface..."
    sudo ip link delete $iface 2>/dev/null || true
}

# Add after print functions
generate_certificates() {
    print_status "Generating SSL certificates..."
    cd "$(dirname "$0")"  # Change to script directory
    chmod +x ../scripts/generate_certs.sh
    bash ../scripts/generate_certs.sh
    if [ $? -ne 0 ]; then
        print_error "Failed to generate certificates!"
        exit 1
    fi
}

# Main execution
case "$1" in
    "server")
        cleanup_interface "tun0"
        generate_certificates
        compile
        print_status "Starting server..."
        sudo ./vpn -i tun0 -s -p 55555
        ;;
    "client")
        cleanup_interface "tun1"
        compile
        print_status "Starting client..."
        sudo ./vpn -i tun1 -c 127.0.0.1 -p 55555
        ;;
    "clean")
        clean
        ;;
    *)
        echo "Usage: $0 {server|client|clean}"
        echo "Commands:"
        echo "  server    - Compile and run as server"
        echo "  client    - Compile and run as client"
        echo "  clean     - Clean build files"
        exit 1
        ;;
esac
