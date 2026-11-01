#include "wal_reader.hpp"
#include "wal_writer.hpp"
#include "wal_record.hpp"
#include "serializer.hpp"

#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>

int main(){
    std::string test_wal_path  = "wal_reader_corrupted_checksum_test.log";
    std::filesystem::remove(test_wal_path);

    storage::WalRecord record1{
        storage::RecordType::PUT, // Type
        1,                        // sequence_number
        1729000000000000ULL,            // timestamp
        "cat",                    // key
        "dog"                     // value
    };
    
    std::vector<std::uint8_t> bytes = storage::serialize(record1);

    bytes[25] ^= 0x01;

    std::ofstream outfile(test_wal_path, std::ios::binary);
    
    outfile.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    outfile.close();

    storage::WalReader reader(test_wal_path);

    storage::WalReadResult result = reader.read_all();

    assert(result.records.empty());

    assert(result.status == storage::WalReadStatus::CORRUPTED);

    assert(result.error_offset.has_value());
    assert(result.error_offset .value()== 0);

    std::cout << "Corrupted checksum detected successfully\n";

    std::filesystem::remove(test_wal_path);
}