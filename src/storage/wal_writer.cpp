#include "wal_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <system_error>
#include <cstdio>
#include <filesystem>

storage::WalWriter::WalWriter(const std::string& filepath){
    this->filepath= filepath;
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
            bytes.size() - bytes_written
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

void storage::WalWriter::replace_records_locked(const std::vector<WalRecord>& records){

    std::string temp_filepath = filepath + ".tmp.XXXXXX";

    int fd2 = ::mkstemp(temp_filepath.data());

    if(fd2 == -1){
        std::cerr<<"[ERROR] Failed to open path: "<<temp_filepath<<"\n";
        throw std::runtime_error("Unable to open the file");
    }

    int flags = ::fcntl(fd2 , F_GETFL);

    if(flags == -1){
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Unable to open the file");
    }

    int res1 = ::fcntl(fd2 , F_SETFL , flags | O_APPEND);

    if(res1 == -1){
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Unable to open the file");
    }

    int fd_flags = ::fcntl(fd2 , F_GETFD);

    if(fd_flags == -1){
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Unable to open the file");
    }

    int res2  = ::fcntl(fd2 , F_SETFD , fd_flags | FD_CLOEXEC);

    if(res2 == -1){
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Unable to open the file");
    }

    for(std::size_t i = 0 ; i < records.size() ; i++){

        std::size_t bytes_written = 0;

        std::vector<std::uint8_t> bytes;
        try {
            bytes = storage::serialize(records[i]);
        } catch (...) {
            close(fd2);
            unlink(temp_filepath.c_str());
            throw;
        }

        while(bytes_written < bytes.size()){
            ssize_t result = ::write(
                fd2,
                bytes.data() + bytes_written,
                bytes.size() - bytes_written
            );

            if(result < 0){
                if(errno == EINTR){
                    continue;
                }
                int saved_errno = errno;
                close(fd2);
                unlink(temp_filepath.c_str());
                throw std::system_error(saved_errno , std::generic_category() , "Temporary WAL write failed");
            }
            if(result == 0){
                close(fd2);
                unlink(temp_filepath.c_str());
                throw std::runtime_error("WAL write made no progress");
            }

            bytes_written += static_cast<std::size_t>(result);
        }


    }

    while(::fdatasync(fd2) == -1){
        if(errno == EINTR){
            continue;
        }
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Temporary WAL synchronization failed");
    }

    int res3 = ::rename(temp_filepath.c_str() , filepath.c_str());
    if(res3 == -1){
        int saved_errno = errno;
        close(fd2);
        unlink(temp_filepath.c_str());
        throw std::system_error(saved_errno , std::generic_category() , "Temp file rename failed");
    }
    int temp = fd;
    fd = fd2;
    close(temp);

    std::filesystem::path parent_dir = std::filesystem::path(filepath).parent_path();

    if(parent_dir.empty()){
        parent_dir = ".";
    }

    int dir_fd = ::open(
        parent_dir.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC
    );

    if(dir_fd == -1){
        failed = true;
        int saved_errno = errno;
        throw std::system_error(saved_errno,std::generic_category(),"Failed to open WAL parent directory");
    }
    while(::fsync(dir_fd) == -1){
        if(errno == EINTR){
            continue;
        }

        failed = true;
        int saved_errno = errno;
        ::close(dir_fd);

        throw std::system_error(saved_errno,std::generic_category(),"WAL directory synchronization failed");
    }

    if(::close(dir_fd) == -1){
        int saved_errno = errno;
        throw std::system_error(saved_errno,std::generic_category(),"Failed to close WAL parent directory");
    }
}