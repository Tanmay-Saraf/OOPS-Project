
#pragma once

#include <string>

namespace oops {

enum class ResponseStatus {
    Success,
    InvalidRequest,
    PermissionDenied,
    NotFound,
    InternalError
};

struct Response {
    ResponseStatus status = ResponseStatus::InternalError;
    std::string message;
    std::string value;
};

} // namespace oops
