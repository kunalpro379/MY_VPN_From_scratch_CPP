#include "VPNConnection.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <filesystem>
VPNConnection::VPNConnection(bool isServer) : isServer_(isServer), listenFd_(-1) {
    tunnel = new Tunnel();
    
    // Get absolute path to certificates
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path cert_dir = current_path.parent_path() / "certs";
    
    // Convert to absolute paths
    std::string cert_path = (cert_dir / "server.crt").string();
    std::string key_path = (cert_dir / "server.key").string();
    
    std::cout << "Using certificates at:\n" 
              << "Cert: " << cert_path << "\n"
              << "Key: " << key_path << std::endl;
              
    Tunnel::setCertificatePaths(cert_path, key_path);
}

VPNConnection::~VPNConnection() {
    if (listenFd_ >= 0) close(listenFd_);
}

bool VPNConnection::connect(const std::string& host, int port) {
    return tunnel->connect(host, std::to_string(port));
}

bool VPNConnection::bind(int port) {
    return setupServer(port);
}

bool VPNConnection::setupServer(int port) {
    std::cout << "Setting up server on port " << port << "..." << std::endl;
    
    // First set up the TCP socket
    if (!setupTCPServer(port)) {
        return false;
    }

    // Pass the socket to the tunnel
    tunnel->set_socket_fd(listenFd_);  // Add this method to Tunnel class

    // Then set up the SSL tunnel
    if (!tunnel->listen(std::to_string(port))) {
        std::cerr << "SSL tunnel setup failed" << std::endl;
        return false;
    }
    std::cout << "SSL tunnel setup complete" << std::endl;

    return true;
}

bool VPNConnection::setupTCPServer(int port) {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    int optval = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = htons(port);

    if (::bind(listenFd_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }
    std::cout << "Successfully bound to port " << port << std::endl;

    if (::listen(listenFd_, SOMAXCONN) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }
    std::cout << "Successfully listening on port " << port << std::endl;

    return true;
}

bool VPNConnection::accept() {
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int client_fd = ::accept(listenFd_, (struct sockaddr*)&client, &len);
    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        return false;
    }
    
    std::cout << "Client connected from " << inet_ntoa(client.sin_addr) << std::endl;
    
    // Initialize SSL for the accepted connection
    if (!tunnel->accept_client(client_fd)) {
        close(client_fd);
        return false;
    }
    
    return true;
}

bool VPNConnection::setupClient(const std::string& host, int port) {
    return tunnel->connect(host, std::to_string(port));
}

ssize_t VPNConnection::read(char* buffer, size_t len) {
    return tunnel->receive(buffer, len);
}

ssize_t VPNConnection::write(const char* buffer, size_t len) {
    return tunnel->send(buffer, len);
}
