
#include "network/TemporaryMemoryEngine.hpp"

namespace oops {

Response TemporaryMemoryEngine::execute(const Request& request) {
    std::lock_guard<std::mutex> lock(mutex_);

    Response response;

    switch (request.command) {
        case Command::Set:
            records_[request.key] = request.value;
            response.status = ResponseStatus::Success;
            response.message = "Value stored";
            break;

        case Command::Get: {
            auto it = records_.find(request.key);

            if (it == records_.end()) {
                response.status = ResponseStatus::NotFound;
                response.message = "Key not found";
            } else {
                response.status = ResponseStatus::Success;
                response.value = it->second;
            }
            break;
        }

        case Command::Delete: {
            auto it = records_.find(request.key);

            if (it == records_.end()) {
                response.status = ResponseStatus::NotFound;
                response.message = "Key not found";
            } else {
                records_.erase(it);
                response.status = ResponseStatus::Success;
                response.message = "Key deleted";
            }
            break;
        }

        case Command::Exists:
            response.status = ResponseStatus::Success;
            response.value = records_.count(request.key) ? "true" : "false";
            break;

        case Command::Keys: {
            response.status = ResponseStatus::Success;

            for (const auto& [key, value] : records_) {
                if (!response.value.empty()) {
                    response.value += " ";
                }
                response.value += key;
            }
            break;
        }

        default:
            response.status = ResponseStatus::InvalidRequest;
            response.message = "Unsupported memory command";
            break;
    }

    return response;
}

}
