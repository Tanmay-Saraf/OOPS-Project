#include "wal_reader.hpp"
#include "serializer.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace storage{
    WalReader::WalReader(const std::string& path) : filepath(path){}
    
    WalReadResult WalReader::read_all() const{
        std::ifstream infile(filepath , std::ios::binary);
        
        if(!infile){
            std::cerr<<"[ERROR] Failed to open the file "<<filepath<<"\n";
            throw std::runtime_error("Unable to open the WAL log file for reading");
        }

        constexpr std::size_t maximum_payload_size = 16*1024*1024;
        constexpr std::size_t fixed_payload_size = 37;
        
        std::size_t offset = 0;

        std::vector<WalRecord>records;

        while(true){
            std::streamsize size = static_cast<std::streamsize>(4);
            std::vector<std::uint8_t> first_length(size);
    
            infile.read(reinterpret_cast<char*>(first_length.data()),size);
    
            if(infile.bad()){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::IO_ERROR,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            if(infile.gcount() == 0 && infile.eof()){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::OK,
                    .error_offset = std::nullopt
                };
            }
    
            if(infile.gcount() < 4 && infile.eof()){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::TRUNCATED,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }

            if(infile.gcount() < 4 ){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::IO_ERROR,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            std::uint32_t payload_size = (static_cast<std::uint32_t>(first_length[0]) << 24)     |
                                            (static_cast<std::uint32_t>(first_length[1]) << 16) |
                                            (static_cast<std::uint32_t>(first_length[2]) << 8)  |
                                            static_cast<std::uint32_t>(first_length[3]);
    
            
    
            if(payload_size < fixed_payload_size){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::CORRUPTED,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            if(payload_size > maximum_payload_size){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::CORRUPTED,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            std::vector<std::uint8_t> buffer(static_cast<std::size_t>(payload_size));
    
            infile.read(reinterpret_cast<char*>(buffer.data()) , payload_size);
    
            if(infile.bad()){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::IO_ERROR,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            if(infile.gcount() < static_cast<std::streamsize>(payload_size) && infile.eof()){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::TRUNCATED,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            if(infile.gcount() < static_cast<std::streamsize>(payload_size)){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::IO_ERROR,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }
    
            std::vector<std::uint8_t> complete_bytes;
            complete_bytes.reserve(first_length.size() + buffer.size());
    
            complete_bytes.insert(complete_bytes.end() , first_length.begin() , first_length.end());
            complete_bytes.insert(complete_bytes.end() , buffer.begin() , buffer.end());
    
            std::optional<WalRecord> record = deserialize(complete_bytes);
    
            if(!record){
                return WalReadResult{
                    .records = records,
                    .status = WalReadStatus::CORRUPTED,
                    .error_offset = static_cast<std::uint64_t>(offset)
                };
            }

            records.push_back(std::move(*record));
            offset += complete_bytes.size();
        }
    }

}