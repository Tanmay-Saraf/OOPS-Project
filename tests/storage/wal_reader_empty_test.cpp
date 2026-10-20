#include "wal_reader.hpp"
#include "wal_record.hpp"

#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <filesystem>

int main(){
    std::string test_wal_path  = "wal_reader_empty_test.log";
    std::filesystem::remove(test_wal_path);

    std::ofstream outfile(test_wal_path);

    outfile.close();
    
    storage::WalReader reader(test_wal_path);

    storage::WalReadResult result = reader.read_all();

    assert(result.records.empty());

    assert(result.status == storage::WalReadStatus::OK);

    assert(!result.error_offset.has_value());
    
    std::cout<<"read successfully";

    std::filesystem::remove(test_wal_path);
}