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
Tunnel::Tunnel() {
    ctx = NULL;
    ssl = NULL;
    socket_fd = -1;
    connected = false;
    SSL_library_init();// Initialize OpenSSL
    OpenSSL_add_all_algorithms();// Load crypto algo
    SSL_load_error_strings();// Loading SSL error messages
}

Tunnel::~Tunnel() {
    disconnect();
}

string Tunnel::certificatePath = "../certs/server.crt";
string Tunnel::privateKeyPath = "../certs/server.key";

bool Tunnel::init_ssl_ctx() {
    const SSL_METHOD* method = TLS_method();
    ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        std::cerr << "Failed to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Set SSL context options
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    // Print file permissions and existence
    std::cout << "Checking certificate files..." << std::endl;
    std::cout << "Certificate path: " << certificatePath 
              << " (exists: " << (access(certificatePath.c_str(), F_OK) == 0) 
              << ", readable: " << (access(certificatePath.c_str(), R_OK) == 0) << ")" << std::endl;
    std::cout << "Private key path: " << privateKeyPath 
              << " (exists: " << (access(privateKeyPath.c_str(), F_OK) == 0)
              << ", readable: " << (access(privateKeyPath.c_str(), R_OK) == 0) << ")" << std::endl;

    // Load certificate
    if (SSL_CTX_use_certificate_file(ctx, certificatePath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load certificate. Error: " << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Load private key
    if (SSL_CTX_use_PrivateKey_file(ctx, privateKeyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load private key. Error: " << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Verify private key
    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Private key verification failed. Error: " << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    std::cout << "SSL context initialized successfully" << std::endl;
    return true;
}

bool Tunnel::open_connection(const std::string& hostname, const std::string& port) {
    struct addrinfo hints = {}, *addrs;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (struct addrinfo* addr = addrs; addr != nullptr; addr = addr->ai_next) {
        socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (socket_fd < 0) continue;

        if (::connect(socket_fd, addr->ai_addr, addr->ai_addrlen) == 0) {  // Added :: to use global connect
            freeaddrinfo(addrs);
            return true;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(addrs);
    return false;
}

bool Tunnel::connect(const std::string& hostname, const std::string& port) {
    if (!init_ssl_ctx()) return false;
    
    if (!open_connection(hostname, port)) {
        std::cerr << "Failed to connect to " << hostname << ":" << port << std::endl;
        return false;
    }

    ssl = SSL_new(ctx);
    if (!ssl) {
        std::cerr << "SSL_new failed" << std::endl;
        return false;
    }

    SSL_set_fd(ssl, socket_fd);
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    connected = true;
    display_certificates();
    return true;
}

bool Tunnel::listen(const std::string& port) {
    std::cout << "Setting up SSL server..." << std::endl;
    
    // Initialize SSL context first
    if (!init_ssl_ctx()) {
        std::cerr << "SSL context initialization failed" << std::endl;
        return false;
    }

    // We don't need to create a new socket here since it's already created in VPNConnection
    // Just use the existing socket_fd
    
    std::cout << "SSL server setup complete" << std::endl;
    return true;
}

bool Tunnel::accept_client(int client_fd) {
    socket_fd = client_fd;
    
    ssl = SSL_new(ctx);
    if (!ssl) {
        std::cerr << "SSL_new failed" << std::endl;
        return false;
    }

    SSL_set_fd(ssl, socket_fd);
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    connected = true;
    display_certificates();
    return true;
}

void Tunnel::disconnect() {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
    }
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
    connected = false;
}

void Tunnel::display_certificates() {
    X509* cert = SSL_get_peer_certificate(ssl);
    if (cert) {
        std::cout << "Server certificates:" << std::endl;
        char* line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        std::cout << "Subject: " << line << std::endl;
        free(line);
        
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        std::cout << "Issuer: " << line << std::endl;
        free(line);
        
        X509_free(cert);
    }
}

ssize_t Tunnel::send(const void* data, size_t length) {
    if (!connected || !ssl) return -1;
    return SSL_write(ssl, data, length);
}

ssize_t Tunnel::receive(void* buffer, size_t length) {
    if (!connected || !ssl) return -1;
    return SSL_read(ssl, buffer, length);
}