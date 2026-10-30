#ifndef SECURITY_LOGGER_H
#define SECURITY_LOGGER_H

#include <fstream>
#include <mutex>
#include <string>

class SecurityLogger {
private:
    std::ofstream logFile;
    std::mutex logMutex;

public:
    explicit SecurityLogger(const std::string& filename);

    SecurityLogger(const SecurityLogger&) = delete;
    SecurityLogger& operator=(const SecurityLogger&) = delete;

    void logEvent(const std::string& event,
                  const std::string& reason);

    ~SecurityLogger();
};

#endif