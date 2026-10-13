#pragma once

#include "common/Request.hpp"
#include <string>

namespace oops {

Request parseRequest(const std::string& line);

} // namespace oops