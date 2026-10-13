
#include "network/RequestParser.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace oops {

Request parseRequest(const std::string& line) {
    Request request;
    std::istringstream input(line);

    std::string command;
    if (!(input >> command)) {
        return request;
    }

    auto hasExtraArgument = [&input]() {
        std::string extra;
        return static_cast<bool>(input >> extra);
    };

    if (command == "SET") {
        if (!(input >> request.key)) {
            return request;
        }

        std::getline(input, request.value);

        const auto first = request.value.find_first_not_of(" \t");
        if (first == std::string::npos) {
            return request;
        }

        request.value.erase(0, first);
        request.command = Command::Set;
    }
    else if (command == "GET") {
        if (!(input >> request.key) || hasExtraArgument()) {
            return request;
        }
        request.command = Command::Get;
    }
    else if (command == "DELETE") {
        if (!(input >> request.key) || hasExtraArgument()) {
            return request;
        }
        request.command = Command::Delete;
    }
    else if (command == "EXISTS") {
        if (!(input >> request.key) || hasExtraArgument()) {
            return request;
        }
        request.command = Command::Exists;
    }
    else if (command == "HISTORY") {
        if (!(input >> request.key) || hasExtraArgument()) {
            return request;
        }
        request.command = Command::History;
    }
    else if (command == "GET_VERSION") {
        std::string versionText;

        if (!(input >> request.key >> versionText) ||
            hasExtraArgument()) {
            return request;
        }

        std::istringstream versionInput(versionText);
        char extra;

        if (!(versionInput >> request.version) ||
            (versionInput >> extra)) {
            return request;
        }

        request.command = Command::GetVersion;
    }
    else if (command == "KEYS" ||
             command == "PING" ||
             command == "QUIT" ||
             command == "SAVE" ||
             command == "RECOVER" ||
             command == "STATUS") {
        if (hasExtraArgument()) {
            return request;
        }

        if (command == "KEYS") {
            request.command = Command::Keys;
        }
        else if (command == "PING") {
            request.command = Command::Ping;
        }
        else if (command == "QUIT") {
            request.command = Command::Quit;
        }
        else if (command == "SAVE") {
            request.command = Command::Save;
        }
        else if (command == "RECOVER") {
            request.command = Command::Recover;
        }
        else {
            request.command = Command::Status;
        }
    }

    return request;
}

} // namespace oops
