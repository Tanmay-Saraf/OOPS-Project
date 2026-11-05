#pragma once
#include <string>
#include <cstdint>

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
    Unlock,
    Login,
    Logout,
    Rollback,
    Replay,
    Invalid
};

struct Request {
    Command command = Command::Invalid;
    std::string key;
    std::string value;
    std::uint64_t version = 0;
    std::uint64_t timestamp = 0;

    // Authentication fields
    std::string username;
    std::string password;
};

}