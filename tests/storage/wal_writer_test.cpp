#include "wal_writer.hpp"


#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(){
    storage::WalRecord record1{
        storage::RecordType::PUT, // Type
        1,                        // sequence_number
        100,                      // transaction_id
        1729000000000,            // timestamp
        "cat",                    // key
        "dog"                     // value
    };

    storage::WalRecord record2{
        storage::RecordType::PUT, // Type
        1,                        // sequence_number
        100,                      // transaction_id
        1729000000000,            // timestamp
        "hello",                  // key
        "world"                   // value
    };

    const std::string test_wal_path = "wal_writer_test.log";

    std::filesystem::remove(test_wal_path);
    
    {
        storage::WalWriter writer(test_wal_path);
        writer.append(record1);
    }

    {
        storage::WalWriter writer(test_wal_path);
        writer.append(record2);
    }

    std::ifstream infile;
    infile.open(test_wal_path , std::ios::binary | std::ios::ate);

    if(!infile){
        std::cerr<<"Error opening the file"<<std::endl;
        return 1;
    }

    const std::streampos end_position = infile.tellg();

    if(end_position < 0 ){
        std::cerr<<"Failed to determine the file size";
        return 1;
    }

    const std::size_t size = static_cast<std::size_t>(end_position);

    infile.seekg(0,std::ios::beg);

    std::vector<uint8_t> buffer(size);

    if(size > 0){
        infile.read(reinterpret_cast<char*>(buffer.data()) , static_cast<std::streamsize>(size));
        if(!infile || infile.gcount() != static_cast<std::streamsize>(size)){
            std::cerr<<"Failed to read the file";
            return 1;
        }
    }

    infile.close();

    std::vector<storage::WalRecord> records;
    std::size_t offset = 0;

    while(offset < buffer.size()){
        if(buffer.size() - offset < 4){
            std::cerr<<"Incomplete record length prefix \n";
            return 1;
        }

        const std::uint32_t payload_size = 
                                        (static_cast<std::uint32_t>(buffer[offset]) << 24)     |
                                        (static_cast<std::uint32_t>(buffer[offset + 1]) << 16) |
                                        (static_cast<std::uint32_t>(buffer[offset + 2]) << 8)  |
                                        static_cast<std::uint32_t>(buffer[offset + 3]);

        const std::size_t record_size = 4 + static_cast<std::size_t>(payload_size);

        if(record_size > buffer.size() - offset){
            std::cerr<<"Incomplete Wal Record";
            return 1;
        }

        std::vector<std::uint8_t> record_bytes(buffer.begin()+offset , buffer.begin()+record_size+offset);

        std::optional<storage::WalRecord> result = storage::deserialize(record_bytes);

        if(!result){
            std::cerr<<"Failed to deserialize \n";
            return 1;
        }

        records.push_back(std::move(*result));
        offset += record_size;
    }

    assert(records.size() == 2);

    assert(records[0].type == record1.type);
    assert(records[0].sequence_number == record1.sequence_number);
    assert(records[0].transaction_id == record1.transaction_id);
    assert(records[0].timestamp == record1.timestamp);
    assert(records[0].key == record1.key);
    assert(records[0].value == record1.value);

    assert(records[1].type == record2.type);
    assert(records[1].sequence_number == record2.sequence_number);
    assert(records[1].transaction_id == record2.transaction_id);
    assert(records[1].timestamp == record2.timestamp);
    assert(records[1].key == record2.key);
    assert(records[1].value == record2.value);

    std::cout<<"Both record written and read successfully";
    std::filesystem::remove(test_wal_path);
    return 0;
}