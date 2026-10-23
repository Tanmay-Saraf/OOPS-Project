#include "wal_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <system_error>

storage::WalWriter::WalWriter(const std::string& filepath){
    fd = ::open(
        filepath.c_str(),
        O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC,
        0644
    );
    if(fd == -1){
        std::cerr<<"[ERROR] Failed to open path: "<<filepath<<"\n";
        throw std::runtime_error("Unable to open the file");
    }
}

void storage::WalWriter::append(const WalRecord& record){
    std::lock_guard<std::mutex> lock(mtx);

    if(failed){
        throw std::runtime_error("Wal writer is in failed state");
    }

    std::vector<std::uint8_t> bytes = storage::serialize(record);

    std::size_t bytes_written = 0;

    while(bytes_written < bytes.size()){
        ssize_t result = ::write(
            fd,
            bytes.data() + bytes_written,
            bytes.size() + bytes_written
        );

        if(result < 0){
            if(errno == EINTR){
                continue;
            }

            failed = true;
            throw std::system_error(errno , std::generic_category() , "WAL write failed");
        }
        if(result == 0){
            failed = true;
            throw std::runtime_error("WAL write made no progress");
        }

        bytes_written += static_cast<std::size_t>(result);
    }


    while(::fdatasync(fd) == -1){
        if(errno == EINTR){
            continue;
        }
        failed = true;
        throw std::system_error(errno , std::generic_category() , "WAL synchronization failed");
    }
}

storage::WalWriter::~WalWriter(){
    if(fd>=0){
        ::close(fd);
    }
}