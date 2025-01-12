#include "Tunnel.hpp"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/err.h>
#include <iostream>
#include <filesystem>
using namespace std;

string Tunnel::certificatePath = "../certs/server.crt";
string Tunnel::privateKeyPath = "../certs/server.key";

Tunnel::Tunnel()
{
    ctx = NULL;
    ssl = NULL;
    socket_fd = -1;
    connected = false;

    // Initialize OpenSSL library components
    SSL_library_init();
    OpenSSL_add_all_algorithms(); // Load crypto algorithms
    SSL_load_error_strings();     // Load SSL error messages
}

Tunnel::~Tunnel()
{
    disconnect();
}

bool Tunnel::init_ssl_ctx()
{
    // TLS_method() provides the most up-to-date TLS version negotiation
    const SSL_METHOD *method = TLS_method();
    ctx = SSL_CTX_new(method);

    if (!ctx)
    {
        cerr << "Failed to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Disable outdated and insecure SSL versions
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    // Enable automatic retry on interrupted operations
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    // Disable certificate verification for testing (should be enabled in production)
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    // Load the server's public certificate from file
    if (SSL_CTX_use_certificate_file(ctx, certificatePath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        cerr << "Failed to load certificate. Error: " << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Load the server's private key used for encryption
    if (SSL_CTX_use_PrivateKey_file(ctx, privateKeyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        cerr << "Failed to load private key. Error: " << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Verify the private key matches the certificate
    if (!SSL_CTX_check_private_key(ctx))
    {
        cerr << "Private key verification failed. Error: " << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    cout << "SSL context initialized successfully" << endl;
    return true;
}

// Establishes TCP connection to remote host
bool Tunnel::open_connection(const string &hostname, const string &port)
{
    // Initialize address info structure for IPv4/IPv6 compatibility
    struct addrinfo hints = {}, *addrs;
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_protocol = IPPROTO_TCP; // TCP protocol
    cout << AF_UNSPEC << endl
         << SOCK_STREAM << endl
         << IPPROTO_TCP << endl;
    cout << hostname << port << endl;
    // Resolve hostname and port to IP addresses
    int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
    cout << status << endl;
    if (status != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        return false;
    }

    // Try each address until we successfully connect
    // for (struct addrinfo *addr = addrs; addr != nullptr; addr = addr->ai_next)
    // {
    // Create a new socket
    socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (socket_fd < 0)
        continue;

    // Attempt to establish TCP connection
    if (::connect(socket_fd, addr->ai_addr, addr->ai_addrlen) == 0)
    { // Added :: to use global connect
        freeaddrinfo(addrs);
        return true;
    }

    close(socket_fd);
    socket_fd = -1;
    // }

    freeaddrinfo(addrs);
    return false;
}

// Creates secure SSL connection over established TCP connection
bool Tunnel::connect(const string &hostname, const string &port)
{
    // First initialize SSL context with certificates and settings
    if (!init_ssl_ctx())
        return false;
    cout << "SSL context initialized successfully" << endl
         << endl;

    // Establish TCP connection to remote host
    if (!open_connection(hostname, port))
    {
        cerr << "Failed to connect to " << hostname << ":" << port << endl;
        return false;
    }

    // Create new SSL structure for this connection
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        cerr << "SSL_new failed" << endl;
        return false;
    }

    // Associate SSL structure with socket file descriptor
    SSL_set_fd(ssl, socket_fd);
    // Perform SSL/TLS handshake
    if (SSL_connect(ssl) != 1)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    connected = true;
    display_certificates();
    return true;
}

bool Tunnel::listen(const string &port)
{
    cout << "Setting up SSL server..." << endl;

    // Initialize SSL context first
    if (!init_ssl_ctx())
    {
        cerr << "SSL context initialization failed" << endl;
        return false;
    }

    // We don't need to create a new socket here since it's already created in VPNConnection
    // Just use the existing socket_fd

    cout << "SSL server setup complete" << endl;
    return true;
}

bool Tunnel::accept_client(int client_fd)
{
    // Store client socket descriptor
    socket_fd = client_fd;

    // Create new SSL structure for this client
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        cerr << "SSL_new failed" << endl;
        return false;
    }

    // Associate SSL with client socket
    SSL_set_fd(ssl, socket_fd);
    // Perform SSL/TLS handshake as server
    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    connected = true;
    display_certificates();
    return true;
}

void Tunnel::disconnect()
{
    // Properly shutdown SSL connection
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }

    // Close underlying TCP socket
    if (socket_fd != -1)
    {
        close(socket_fd);
        socket_fd = -1;
    }

    // Free SSL context and associated resources
    if (ctx)
    {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
    connected = false;
}

void Tunnel::display_certificates()
{
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert)
    {
        cout << "Server certificates:" << endl;
        char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        cout << "Subject: " << line << endl;
        free(line);

        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        cout << "Issuer: " << line << endl;
        free(line);

        X509_free(cert);
    }
}

ssize_t Tunnel::send(const void *data, size_t length)
{
    if (!connected || !ssl)
        return -1;
    return SSL_write(ssl, data, length);
}

ssize_t Tunnel::receive(void *buffer, size_t length)
{
    if (!connected || !ssl)
        return -1;
    return SSL_read(ssl, buffer, length);
}