#include "wal_writer.hpp"

#include <iostream>
#include <stdexcept>


storage::WalWriter::WalWriter(const std::string& filepath){
    outFile.open(filepath , std::ios::app | std::ios::binary);
    if(!outFile.is_open()){
        std::cerr<<"[ERROR] Failed to open path: "<<filepath<<"\n";
        throw std::runtime_error("Unable to open the file");
    }
}

void storage::WalWriter::append(const WalRecord& record){
    std::vector<std::uint8_t> bytes = storage::serialize(record);
    std::lock_guard<std::mutex> lock(mtx);

    if(failed){
        throw std::runtime_error("Wal writer is in failed state");
    }

    outFile.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));

    if(!outFile){
        failed = true;
        std::cerr<<"[ERROR] Failed to complete the write \n";
        throw std::runtime_error("Unable to complete the write operation");
    }
}