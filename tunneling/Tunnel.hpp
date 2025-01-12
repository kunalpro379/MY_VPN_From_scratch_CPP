#pragma once

#include <string>
#include <memory>
#include <openssl/ssl.h>
using namespace std;

// Tunnel class: Handles secure SSL/TLS communication between endpoints
class Tunnel
{
public:
    Tunnel();
    ~Tunnel();

    bool connect(const string &hostname, const string &port);

    bool listen(const string &port);

    // Accepts an incoming client connection and establishes SSL

    bool accept_client(int client_fd);

    // Returns the underlying socket file descriptor
    int get_socket_fd() const { return socket_fd; }

    // Sends data through the secure tunnel

    ssize_t send(const void *data, size_t length);

    // Receives data from the secure tunnel

    ssize_t receive(void *buffer, size_t length);

    // Checks if the tunnel is currently connected
    bool is_connected() const { return connected; }

    // Closes the secure connection and cleans up resources
    void disconnect();

    // Sets the socket file descriptor for the tunnel
    void set_socket_fd(int fd) { socket_fd = fd; }

    // Static method to set certificate paths globally
    static void setCertificatePaths(const string &certPath, const string &keyPath);

    // Instance method to set certificate paths for this tunnel
    bool setCertificates(const string &certPath, const string &keyPath);

private:
    // Initializes the SSL context with proper settings
    bool init_ssl_ctx();

    // Establishes the TCP connection to remote host
    bool open_connection(const string &hostname, const string &port);

    // Displays SSL certificate information for debugging
    void display_certificates();

    // SSL context for the connection
    SSL_CTX *ctx;
    // SSL connection instance
    SSL *ssl;
    // Socket file descriptor
    int socket_fd;
    // Connection state flag
    bool connected;

    // Static paths for SSL certificates
    static string certificatePath;
    static string privateKeyPath;

    // Delete copy constructor and assignment operator to prevent copying
    Tunnel(const Tunnel &) = delete;
    Tunnel &operator=(const Tunnel &) = delete;
};
