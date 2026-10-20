#pragma once
#include <chrono>
#include <cstdint>
#include <string>

enum class Operation {
    Put,
    Delete
};

struct Version {
    std::uint64_t id;
    std::string value;
    Operation operation;
    std::chrono::system_clock::time_point timestamp;

    Version(std::uint64_t id, const std::string &value, Operation operation, std::chrono::system_clock::time_point timestamp)
    : id(id), value(value), operation(operation), timestamp(timestamp){}
};