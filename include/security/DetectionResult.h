#ifndef DETECTION_H
#define DETECTION_H
#include<string>

enum class Severity{
    LOW, MEDIUM , HIGH , CRITICAL
};
struct DetectionResult{
    std::string Detector;
    int Score;
    bool Suspicious_flag;
    std::string reason;
    Severity HowSevere;
};

#endif