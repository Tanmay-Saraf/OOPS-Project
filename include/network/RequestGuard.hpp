
#pragma once

#include "common/Request.hpp"

#include <string>

namespace oops {

    struct GuardDecision {

        bool allowed = true;
        std::string reason;

    };

    class RequestGuard {

    public:

        virtual ~RequestGuard() = default;
        virtual GuardDecision check(const Request& request) = 0;

    };

    class AllowAllGuard : public RequestGuard {

    public:

        GuardDecision check(const Request&) override {

            return GuardDecision{};

        }

    };

}
