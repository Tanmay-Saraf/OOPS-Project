#include "wal_reader.hpp"
#include "wal_writer.hpp"
#include "wal_record.hpp"

#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <filesystem>

int main(){
    std::string test_wal_path  = "wal_reader_test.log";
    std::filesystem::remove(test_wal_path);

    storage::WalRecord record1{
        storage::RecordType::PUT, // Type
        1,                        // sequence_number
        1729000000000000ULL,            // timestamp
        "cat",                    // key
        "dog"                     // value
    };

    storage::WalRecord record2{
        storage::RecordType::PUT, // Type
        2,                        // sequence_number
        1729000000000000ULL,            // timestamp
        "hello",                  // key
        "world"                   // value
    };
    
    {
        storage::WalWriter writer(test_wal_path);
        writer.append(record1);
    }

    {
        storage::WalWriter writer(test_wal_path);
        writer.append(record2);
    }

    storage::WalReader reader(test_wal_path);

    storage::WalReadResult result = reader.read_all();

    assert(result.status == storage::WalReadStatus::OK);
    assert(result.records.size() == 2);

    assert(result.records[0].type == record1.type);
    assert(result.records[0].sequence_number == record1.sequence_number);
    assert(result.records[0].timestamp == record1.timestamp);
    assert(result.records[0].key == record1.key);
    assert(result.records[0].value == record1.value);

    assert(result.records[1].type == record2.type);
    assert(result.records[1].sequence_number == record2.sequence_number);
    assert(result.records[1].timestamp == record2.timestamp);
    assert(result.records[1].key == record2.key);
    assert(result.records[1].value == record2.value);

    std::cout<<"Both read successfully";

    assert(!result.error_offset.has_value());
    std::filesystem::remove(test_wal_path);
}