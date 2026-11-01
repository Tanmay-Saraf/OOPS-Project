#pragma once

namespace oops {

class UniqueFd {
public:

    UniqueFd() noexcept = default;
    UniqueFd(int fd);
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    ~UniqueFd();

    UniqueFd(UniqueFd&& other) noexcept;
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    int get() const noexcept {
        return fd_;
    }

private:
    int fd_ = -1;
};

}