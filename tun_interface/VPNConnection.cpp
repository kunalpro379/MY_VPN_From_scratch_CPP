#include "VPNConnection.hpp" 
#include "Logger.hpp" 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <netdb.h>
#include <string.h> 
#include <iostream> 
#include <filesystem>

using namespace std; 

VPNConnection::VPNConnection(bool isServer) : isServer_(isServer), listenFd_(-1) // Constructor to initialize the VPN connection
{
    tunnel = make_unique<Tunnel>(); // Create a unique pointer to the Tunnel object

    filesystem::path current_path = filesystem::current_path(); 
    filesystem::path cert_base = current_path.parent_path(); 
    filesystem::path cert_dir; 

    if (isServer_) // Check if the connection is a server
    {
        cert_dir = cert_base / "server_certs"; 
        cout << "Using server certificates from: " << cert_dir << endl;
    }
    else // If the connection is a client
    {
        cert_dir = cert_base / "client_certs"; 
        cout << "Using client certificates from: " << cert_dir << endl; 
    }

    string cert_path = (cert_dir / (isServer_ ? "server.crt" : "client.crt")).string(); 
    string key_path = (cert_dir / (isServer_ ? "server.key" : "client.key")).string(); 

    cout << "Certificate paths:\n"
         << "Cert: " << cert_path << "\n"
         << "Key: " << key_path << endl; 

    Tunnel::setCertificatePaths(cert_path, key_path); 
}

VPNConnection::~VPNConnection() 
{
    if (listenFd_ >= 0) 
        close(listenFd_); 
}

bool VPNConnection::connect(const string &host, int port) 
{
    cout << "Attempting VPN connection to " << host << ":" << port << endl; 

    // // Verify certificates exist before attempting connection
    // filesystem::path cert_dir = filesystem::current_path().parent_path() / "client_certs"; // Get the client certificate directory
    // string client_cert = (cert_dir / "client.crt").string(); // Set the client certificate path
    // string client_key = (cert_dir / "client.key").string(); // Set the client key path
    // string ca_cert = (cert_dir / "ca.crt").string(); // Set the CA certificate path

    cout << "Verifying certificate files..." << endl; // Print the verification message
    if (!filesystem::exists(client_cert) || !filesystem::exists(client_key) || !filesystem::exists(ca_cert)) // Check if the certificate files exist
    {
        cerr << "Missing certificate files. Please run generate_certs.sh first" << endl; 
        return false; // Return false if the certificate files are missing
    }

    // const int MAX_RETRIES = 3;
    // const int BASE_DELAY = 2; 

    // for (int i = 0; i < MAX_RETRIES; i++) // Loop through the retries
    // {
    //     cout << "\nConnection attempt " << (i + 1) << " of " << MAX_RETRIES << endl; 

        if (tunnel->connect(host, to_string(port))) 
        {
            cout << "VPN connection established!" << endl; 
            return true; 
        }

        if (i < MAX_RETRIES - 1) 
        {
            int delay = BASE_DELAY * (i + 1); 
            cout << "Connection failed, waiting " << delay << " seconds before retry..." << endl; 
            sleep(delay); 
        }
    // }

    cerr << "Failed to establish VPN connection after " << MAX_RETRIES << " attempts" << endl; 
    return false; 
}

bool VPNConnection::bind(int port) // Bind to a port for listening
{
    return setupServer(port);
}

bool VPNConnection::setupServer(int port)
{
    cout << "Setting up server on port " << port << "..." << endl; // Print the setup message

    // First set up the TCP socket
    if (!setupTCPServer(port)) 
    {
        return false; 
    }

    // Pass the socket to the tunnel
    tunnel->set_socket_fd(listenFd_); 

    // Then set up the SSL tunnel
    if (!tunnel->listen(to_string(port))) // Set up the SSL tunnel
    {
        cerr << "SSL tunnel setup failed" << endl; 
        return false; 
    }
    cout << "SSL tunnel setup complete" << endl; 

    return true; 
}

bool VPNConnection::setupTCPServer(int port) 
{
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0); 
    if (listenFd_ < 0) 
    {
        cerr << "Failed to create socket: " << strerror(errno) << endl; 
        return false;
    }

    int optval = 1; 
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
    {
        cerr << "Failed to set socket options: " << strerror(errno) << endl; 
        close(listenFd_); 
        return false; 
    }

    memset(&addr_, 0, sizeof(addr_)); 
    addr_.sin_family = AF_INET; 
    addr_.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr_.sin_port = htons(port);

    if (::bind(listenFd_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0) // Bind the socket to the address
    {
        cerr << "Bind failed: " << strerror(errno) << endl; 
        close(listenFd_);
        return false; 
    }
    cout << "Successfully bound to port " << port << endl; 

    if (::listen(listenFd_, SOMAXCONN) < 0) 
    {
        cerr << "Listen failed: " << strerror(errno) << endl; 
        close(listenFd_); 
        return false; 
    }
    cout << "Successfully listening on port " << port << endl; 

    return true; 
}

bool VPNConnection::accept()
{
    cout << "Waiting for client connection..." << endl; 
    struct sockaddr_in client; 
    socklen_t len = sizeof(client); 
    int client_fd = ::accept(listenFd_, (struct sockaddr *)&client, &len);
    if (client_fd < 0) 
    {
        cerr << "Accept failed: " << strerror(errno) << endl; 
        return false; 
    }

    cout << "Client connected from " << inet_ntoa(client.sin_addr) << endl; 
    cout << "Initializing SSL connection..." << endl;

    if (!tunnel->accept_client(client_fd))
    {
        cerr << "SSL connection failed" << endl; 
        close(client_fd); 
        return false; 
    }

    cout << "Client connection fully established" << endl; 
    return true; 
}

bool VPNConnection::setupClient(const string &host, int port) 
{
    return tunnel->connect(host, to_string(port)); 
}

ssize_t VPNConnection::read(char *buffer, size_t len) 
{
    return tunnel->receive(buffer, len); 
}

ssize_t VPNConnection::write(const char *buffer, size_t len) 
{
    return tunnel->send(buffer, len); // Send data in the Tunnel class
}

bool VPNConnection::configureCertificates(const string &certPath, const string &keyPath) // Configure certificates for the VPN connection
{
    if (!tunnel) // Check if the Tunnel object is initialized
    {
        cerr << "Tunnel not initialized" << endl; 
        return false; 
    }

    certPath_ = certPath; 
    keyPath_ = keyPath; 

    Tunnel::setCertificatePaths(certPath, keyPath); 

    cout << "Successfully configured certificates:\n"
         << "Cert: " << certPath << "\n"
         << "Key: " << keyPath << endl;

    return true; 
}
