#pragma once  // Ensure header is only included once
#include <string>

// Class to manage a TUN network interface device
class TunDevice {
public:
    // Maximum buffer size for reading/writing network packets
    static constexpr size_t BUFFER_SIZE = 2000;

    TunDevice(const std::string& name, bool isServer);
    
    ~TunDevice();

    bool initialize();
    
    bool configureInterface(const std::string& ip);
    
    // Reads network packets from the TUN device
    ssize_t read(char* buffer, size_t len);
    
    // Writes network packets to the TUN device
    ssize_t write(const char* buffer, size_t len);
    
    // Returns the file descriptor of the TUN device
    int getFd() const { return fd_; }

private:
    std::string name_;    // TUN interface
    int fd_;             // File descriptor for the TUN device
    bool isServer_;     
};
