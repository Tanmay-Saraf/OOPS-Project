#ifndef SECURITY_LOGGER_H
#define SECURITY_LOGGER_H
#include <string>
#include<fstream>
class SecurityLogger {
    private :
        std::ofstream logFile;
    public:
        explicit SecurityLogger (const std::string &filename);
        void logEvent(const std::string& event,const std::string& reason);
        
        ~SecurityLogger();
    };

#endif 