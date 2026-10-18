#include "serializer.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <optional>

int main() {
    storage::WalRecord record{
        storage::RecordType::PUT, // Type
        1,                        // sequence_number
        100,                      // transaction_id
        1729000000000,            // timestamp
        "cat",                    // key
        "dog"                     // value
    };

    // Serialize the record into bytes
    std::vector<std::uint8_t> bytes = storage::serialize(record);

    constexpr std::size_t expected_size = 47;
    assert(bytes.size() == expected_size);

    assert(bytes[4] == static_cast<std::uint8_t>(storage::RecordType::PUT));

    assert(bytes[29] == 0);
    assert(bytes[30] == 0);
    assert(bytes[31] == 0);
    assert(bytes[32] == 3);

    assert(bytes[33] == 'c');
    assert(bytes[34] == 'a');
    assert(bytes[35] == 't');

    assert(bytes[36] == 0);
    assert(bytes[37] == 0);
    assert(bytes[38] == 0);
    assert(bytes[39] == 3);

    assert(bytes[40] == 'd');
    assert(bytes[41] == 'o');
    assert(bytes[42] == 'g');

    
    // Deserialize the bytes into record
    std::optional<storage::WalRecord> resultant_record = storage::deserialize(bytes);
    
    assert(resultant_record);
    
    assert(resultant_record->type == record.type);
    assert(resultant_record->sequence_number == record.sequence_number);
    assert(resultant_record->transaction_id == record.transaction_id);
    assert(resultant_record->timestamp == record.timestamp);
    assert(resultant_record->key == record.key);
    assert(resultant_record->value == record.value);


    // Corrupted record test

    std::vector<std::uint8_t> corrupted_bytes = bytes;
    corrupted_bytes[33] ^= 1;
    
    std::optional<storage::WalRecord> corrupted_record = storage::deserialize(corrupted_bytes);

    assert(!corrupted_record);

    // Truncated record test
    std::vector<std::uint8_t> truncated_bytes = bytes;
    truncated_bytes.pop_back();

    std::optional<storage::WalRecord> truncated_record = storage::deserialize(truncated_bytes);

    assert(!truncated_record);

    // Invalid length test
    std::vector<std::uint8_t> invalid_length_bytes = bytes;
    for(int i = 0 ; i < 4 ; i++){
        invalid_length_bytes[i] = 0;
    }

    std::optional<storage::WalRecord> invalid_length_record = storage::deserialize(invalid_length_bytes);

    assert(!invalid_length_record);

    // Invalid type test
    std::vector<std::uint8_t> invalid_type_bytes = bytes;
    invalid_type_bytes[4] = static_cast<uint8_t>(255);

    std::optional<storage::WalRecord> invalid_type_record = storage::deserialize(invalid_type_bytes);

    assert(!invalid_type_record);
    
    std::cout << "All serializer tests passed!\n";
    return 0;
}
