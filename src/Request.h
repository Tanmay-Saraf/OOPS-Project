#ifndef REQUEST_H
#define REQUEST_H
#include <string>
#include <cstdint>
enum class Operation {
    GET,
    SET,
    DELETE_RECORD
};
enum class RequestSource {
    CLIENT,
    ADMIN,
    RECOVERY,
    INTERNAL
};
struct Request {
    uint64_t requestId;
    std::string clientId;
    RequestSource source;
    Operation operation;
    std::string recordId;
    std::string data;
    uint64_t timestamp;
};
#endif

