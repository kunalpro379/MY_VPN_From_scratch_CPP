#pragma once
#include <string>

class TunDevice {
public:
    static constexpr size_t BUFFER_SIZE = 2000;

    TunDevice(const std::string& name, bool isServer);
    ~TunDevice();

    bool initialize();
    bool configureInterface(const std::string& ip);
    ssize_t read(char* buffer, size_t len);
    ssize_t write(const char* buffer, size_t len);
    int getFd() const { return fd_; }

private:
    std::string name_;
    int fd_;
    bool isServer_;
};
