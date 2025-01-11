#pragma once
#include <string>

class Logger {
public:
    static void setDebug(bool enable) { debug_ = enable; }
    static void debug(const std::string& msg);
    static void error(const std::string& msg);

private:
    static bool debug_;
};
