#pragma once
#include <string>
#include <netinet/in.h>
#include <memory>
#include "tunneling/Tunnel.hpp"
using namespace std;

class VPNConnection
{
public:
    // Main VPN connection interface
    VPNConnection(bool isServer);
    ~VPNConnection();

    // Core networking operations
    bool connect(const string &host, int port);
    bool bind(int port);
    bool accept();
    ssize_t read(char *buffer, size_t len);
    ssize_t write(const char *buffer, size_t len);
    int getFd() const { return tunnel ? tunnel->get_socket_fd() : -1; }

    // Network configuration methods
    bool setupRouting();
    bool cleanupRouting();
    bool configureCertificates(const string &certPath, const string &keyPath);

private:
    // Connection setup helpers
    bool setupServer(int port);
    bool setupTCPServer(int port);
    bool setupClient(const string &host, int port);
    bool configureIPForwarding();
    bool setupNATRules();
    string getDefaultGateway() const;

    // Member variables
    bool isServer_;
    int listenFd_;
    struct sockaddr_in addr_;
    unique_ptr<Tunnel> tunnel;
    static constexpr size_t BUFFER_SIZE = 2000;
    string certPath_;
    string keyPath_;
};
