#include<network/UniqueFd.hpp>
#include <unistd.h>

namespace oops {
    
    UniqueFd::UniqueFd(int fd) : fd_(fd) {}

    UniqueFd::~UniqueFd(){
        if(fd_ != -1){
            close(fd_);
        }
    }

    UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {

        if(this != &other){
            if(fd_ != -1){
                close(fd_);
            }
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
        
    }

} // namespace oops
