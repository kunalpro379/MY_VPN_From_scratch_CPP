#pragma once

#include <string>
#include <memory>
#include <openssl/ssl.h>

class Tunnel {
public:
    Tunnel();
    ~Tunnel();
    
    bool connect(const std::string& hostname, const std::string& port);
    bool listen(const std::string& port);
    bool accept_client(int client_fd);
    int get_socket_fd() const { return socket_fd; }
    
    ssize_t send(const void* data, size_t length);
    ssize_t receive(void* buffer, size_t length);
    
    bool is_connected() const { return connected; }
    void disconnect();
    void set_socket_fd(int fd) { socket_fd = fd; }

    static void setCertificatePaths(const std::string& certPath, const std::string& keyPath) {
        certificatePath = certPath;
        privateKeyPath = keyPath;
    }

private:
    bool init_ssl_ctx();
    bool open_connection(const std::string& hostname, const std::string& port);
    void display_certificates();

    SSL_CTX* ctx;
    SSL* ssl;
    int socket_fd;
    bool connected;
    
    static std::string certificatePath;
    static std::string privateKeyPath;

    // Prevent copying
    Tunnel(const Tunnel&) = delete;
    Tunnel& operator=(const Tunnel&) = delete;
};
