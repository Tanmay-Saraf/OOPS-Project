#pragma once
#include <string>

namespace oops {

enum class Command {
    Set,
    Get,
    Delete,
    Exists,
    Keys,
    History,
    GetVersion,
    Ping,
    Quit,
    Save,
    Recover,
    Status,
    Invalid
};

struct Request {
    Command command = Command::Invalid;
    std::string key;
};

} // namespace oops

