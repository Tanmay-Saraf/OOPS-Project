#include "CanaryDetector.h"
CanaryDetector::CanaryDetector(){
    canaryRecords.insert("canary_001");
    canaryRecords.insert("canary_002");
    canaryRecords.insert("canary_003");
}
DetectionResult CanaryDetector :: analyze(const Request& request){
    DetectionResult result;
    result.Detector = "Canary Detector";
    result.Score = 0;
    result.reason = "Normal Activity";
    result.Suspicious_flag = false;
    result.HowSevere = Severity::LOW;
    if(canaryRecords.find(request.recordId)!=canaryRecords.end()&&
        (request.operation == Operation::SET|| 
            request.operation == Operation::DELETE_RECORD)){
        result.Score = 100;
        result.Suspicious_flag = true;
        result.reason = "Attempt to modify or delete canary records";
        result.HowSevere = Severity::CRITICAL;        
    }
    return result; 
}
