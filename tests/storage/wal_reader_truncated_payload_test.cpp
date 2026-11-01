#include "wal_reader.hpp"
#include "wal_record.hpp"

#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>

int main(){
    std::string test_wal_path = "wal_reader_truncated_payload_test.log";
    std::filesystem::remove(test_wal_path);

    std::ofstream outfile(test_wal_path, std::ios::binary);

    std::vector<std::uint8_t>buffer = {0,0,0,29,1,1,1,1,1,1};
    outfile.write(reinterpret_cast<const char*>(buffer.data()),static_cast<std::streamsize>(buffer.size())); // prefix says 37 but only added 6
    
    outfile.close();
    
    storage::WalReader reader(test_wal_path);

    storage::WalReadResult result = reader.read_all();

    assert(result.records.empty());

    assert(result.status == storage::WalReadStatus::TRUNCATED);

    assert(result.error_offset.has_value());
    assert(result.error_offset.value() == 0);
    
    std::cout<<"read successfully";

    std::filesystem::remove(test_wal_path);
}