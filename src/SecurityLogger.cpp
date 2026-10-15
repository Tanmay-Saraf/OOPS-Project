#include "SecurityLogger.h"
#include <chrono>
#include <iomanip>
#include <ctime>

SecurityLogger ::SecurityLogger(const std::string &filename){
    logFile.open(filename,std::ios::app);
}
void SecurityLogger::logEvent(const std::string &event ,const std::string &reason){
    if(!logFile.is_open()){
        return;
    }
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    logFile << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " | " << event << " | " << reason << '\n';
    logFile.flush();
}
SecurityLogger::~SecurityLogger(){
    if(logFile.is_open()){
        logFile.close();
    }
}