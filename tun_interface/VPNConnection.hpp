#pragma once
#include <string>
#include <netinet/in.h>
#include <memory>
#include "tunneling/Tunnel.hpp"  // Updated include path

class VPNConnection {
public:
    VPNConnection(bool isServer);
    ~VPNConnection();

    bool connect(const std::string& host, int port);
    bool bind(int port);
    bool accept();
    ssize_t read(char* buffer, size_t len);
    ssize_t write(const char* buffer, size_t len);
    int getFd() const { return tunnel ? tunnel->get_socket_fd() : -1; }
    bool setupRouting();
    bool cleanupRouting();

private:
    bool setupServer(int port);
    bool setupTCPServer(int port);  // Add this line
    bool setupClient(const std::string& host, int port);
    bool configureIPForwarding();
    bool setupNATRules();
    std::string getDefaultGateway() const;
    
    bool isServer_;
    int listenFd_;
    struct sockaddr_in addr_;
    std::unique_ptr<Tunnel> tunnel;
    static constexpr size_t BUFFER_SIZE = 2000;
};
