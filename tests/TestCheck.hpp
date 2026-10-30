#pragma once

#include <cstdlib>
#include <iostream>

// Unlike assert(), TEST_CHECK remains active when NDEBUG is defined.
#define TEST_CHECK(...)                                                     \
    do {                                                                    \
        if (!static_cast<bool>((__VA_ARGS__))) {                            \
            std::cerr << "TEST CHECK FAILED: " << #__VA_ARGS__              \
                      << "\n  at " << __FILE__ << ':' << __LINE__       \
                      << std::endl;                                         \
            std::abort();                                                   \
        }                                                                   \
    } while (false)
