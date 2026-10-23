
#pragma once

#include "common/Request.hpp"
#include "common/Response.hpp"

namespace oops {

class MemoryEngineAdapter {
public:
    virtual ~MemoryEngineAdapter() = default;

    virtual Response execute(const Request& request) = 0;
};

}
