
#include "network/ResponseSerializer.hpp"

namespace oops {

std::string serializeResponse(const Response& response) {
    std::string serialized;

    switch (response.status) {
        case ResponseStatus::Success:
            serialized = "OK";
            break;

        case ResponseStatus::InvalidRequest:
            serialized = "ERROR INVALID_REQUEST";
            break;

        case ResponseStatus::PermissionDenied:
            serialized = "ERROR PERMISSION_DENIED";
            break;

        case ResponseStatus::NotFound:
            serialized = "ERROR NOT_FOUND";
            break;

        case ResponseStatus::InternalError:
        default:
            serialized = "ERROR INTERNAL_ERROR";
            break;
    }

    const std::string& detail = !response.value.empty() ? response.value : response.message;

    if (!detail.empty()) {
        serialized += " ";
        serialized += detail;
    }

    serialized += '\n';
    return serialized;
}

}
