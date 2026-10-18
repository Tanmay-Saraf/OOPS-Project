#include "security/SecurityLogger.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

SecurityLogger::SecurityLogger(const std::string& filename)
    : logFile(filename, std::ios::out | std::ios::app) {
    if (!logFile.is_open()) {
        throw std::runtime_error("Failed to open security log: " + filename);
    }
}

void SecurityLogger::logEvent(
    const std::string& event,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!logFile.is_open() || !logFile.good()) {
        throw std::runtime_error("Security log is unavailable");
    }

    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto dayPoint = floor<days>(now);
    const year_month_day date{dayPoint};
    const hh_mm_ss timeOfDay{now - dayPoint};

    std::ostringstream timestamp;

    timestamp << std::setfill('0')
              << std::setw(4) << int(date.year()) << '-'
              << std::setw(2) << unsigned(date.month()) << '-'
              << std::setw(2) << unsigned(date.day()) << ' '
              << std::setw(2) << timeOfDay.hours().count() << ':'
              << std::setw(2) << timeOfDay.minutes().count() << ':'
              << std::setw(2) << timeOfDay.seconds().count()
              << " UTC";

    logFile << timestamp.str()
            << " | " << event
            << " | " << reason << '\n';

    logFile.flush();

    if (!logFile) {
        throw std::runtime_error("Failed to write security log");
    }
}

SecurityLogger::~SecurityLogger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}