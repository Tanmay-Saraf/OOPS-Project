#include "serializer.hpp"
#include <stdexcept>

namespace {

    void convert64(std::vector<std::uint8_t>& bytes , std::uint64_t val){
        for(int i = 56 ; i >=0 ; i = i-8){
            std::uint64_t shifted_value = val>>i;
            std::uint8_t result = shifted_value & 255;
            bytes.push_back(result);
        }
    }

    void convert32(std::vector<std::uint8_t>& bytes , std::uint32_t val){
        for(int i = 24 ; i >=0 ; i = i-8){
            std::uint32_t shifted_value = val>>i;
            std::uint8_t result = shifted_value & 255;
            bytes.push_back(result);
        }
    }

    std::uint32_t read32(const std::vector<std::uint8_t>&bytes , std::size_t offset){
        std::uint32_t result = static_cast<uint32_t>(bytes[offset])<<24   |
                          static_cast<uint32_t>(bytes[offset+1])<<16 |
                          static_cast<uint32_t>(bytes[offset+2])<<8 | 
                          static_cast<uint32_t>(bytes[offset+3]);
        return result;
    }

    std::uint64_t read64(const std::vector<std::uint8_t>&bytes , std::size_t offset){
        std::uint64_t result = static_cast<uint64_t>(bytes[offset])<<56   | 
                          static_cast<uint64_t>(bytes[offset+1])<<48 |
                          static_cast<uint64_t>(bytes[offset+2])<<40 |
                          static_cast<uint64_t>(bytes[offset+3])<<32 |
                          static_cast<uint64_t>(bytes[offset+4])<<24 |
                          static_cast<uint64_t>(bytes[offset+5])<<16 |
                          static_cast<uint64_t>(bytes[offset+6])<<8 |
                          static_cast<uint64_t>(bytes[offset+7]);
        return result;
    }

    std::uint32_t calculate_crc32(const std::vector<std::uint8_t>& bytes){
        std::uint32_t crc = 0xFFFFFFFF;
        constexpr std::uint32_t poly = 0xEDB88320;
        for(std::size_t bytes_idx = 0 ; bytes_idx < bytes.size() ; bytes_idx++){
            crc ^= bytes[bytes_idx];
            for(int bit_index = 0 ; bit_index < 8 ; bit_index++){
                std::uint8_t last_bit = crc & 1;
                crc = crc>>1;
                if(last_bit == 1){
                    crc = crc^poly;
                }
            }
        }

        return crc^0xFFFFFFFF;
    }

}

namespace storage{

    std::vector<std::uint8_t> serialize(const WalRecord& record){
        // Constant data use constexpr , so it will be compile time constant 
        constexpr std::size_t maximum_payload_size = 16*1024*1024;
        constexpr std::size_t fixed_payload_size = 37;
        
        // Validating the type
        switch(record.type){
            case RecordType::PUT:
            case RecordType::DELETE:
            case RecordType::BEGIN:
            case RecordType::COMMIT:
            break;
            default:
            throw std::invalid_argument("No such type is found");
        }
        // Calculated size of key and value
        std::size_t key_size = record.key.size();
        std::size_t value_size = record.value.size();

        // Validation as maximum size allowed is 16MiB
        if(key_size > maximum_payload_size - fixed_payload_size){
            throw std::invalid_argument("Payload size cant be greater than 16 MiB");
        }
        
        if(value_size > maximum_payload_size - fixed_payload_size - key_size){
            throw std::invalid_argument("Payload size cant be greater than 16 MiB");
        }

        //Final size
        std::size_t payload_size = fixed_payload_size + key_size + value_size;

        std::vector<std::uint8_t> bytes;

        // Reserving bytes for the vector
        bytes.reserve(4+payload_size);

        std::uint32_t prefix_length = payload_size;

        // First adding the prefix length or the record length which is 32 bits big 
        convert32(bytes , prefix_length);

        // Adding the record type
        bytes.push_back(static_cast<std::uint8_t>(record.type));
        
        // Adding sequence number which is 64 bits big
        convert64(bytes , record.sequence_number);
    
        // Adding transaction id which is 64 bits big
        convert64(bytes , record.transaction_id);

        // Adding timestamp which is 64 bits big
        convert64(bytes , record.timestamp);

        // Adding key size which is 32 bits big
        std::uint32_t key_size32 = key_size;
        convert32(bytes , key_size32);

        // Inserting actual key value
        bytes.insert(bytes.end() , record.key.begin() , record.key.end());
        
        // Adding value size which is 32 bits big
        std::uint32_t value_size32 = value_size;
        convert32(bytes , value_size32);

        // Inserting actual value chars
        bytes.insert(bytes.end() , record.value.begin() , record.value.end());

        // Calculating the check sum using CRC-32/ISO-HDLC
        std::uint32_t checksum = calculate_crc32(bytes);

        // Inserting the checksum into the vector which is 32 bits big
        convert32(bytes , checksum);

        return bytes;
    }
    
    std::optional<WalRecord> deserialize(const std::vector<std::uint8_t>&bytes){

        // Calculate total bytes of the vector 
        std::size_t total_bytes = bytes.size();

        // Validate if bytes even contain prefix length
        if(total_bytes < 4){
            return std::nullopt;
        }

        // Calculate payload size
        std::uint32_t payload_size = read32(bytes , 0);

        // Constant values
        constexpr std::size_t maximum_payload_size = 16*1024*1024;
        constexpr std::size_t fixed_payload_size = 37;

        // Validation of payload size if it contains all the fields 
        if(payload_size < fixed_payload_size || payload_size > maximum_payload_size){
            return std::nullopt;
        }

        if(total_bytes != 4 + static_cast<std::size_t>(payload_size)){
            return std::nullopt;
        }

        // Type 
        std::uint8_t type = bytes[4];

        // Validation of type
        switch (type)
        {
        case static_cast<std::uint8_t>(RecordType::PUT):
        case static_cast<std::uint8_t>(RecordType::DELETE):
        case static_cast<std::uint8_t>(RecordType::BEGIN):
        case static_cast<std::uint8_t>(RecordType::COMMIT):
            break;
        default:
            return std::nullopt;
        }

        // Sequence number 
        std::uint64_t sequence_number = read64(bytes , 5);

        // Transaction id 
        std::uint64_t transaction_id = read64(bytes , 13);

        // Timestamp
        std::uint64_t timestamp = read64(bytes , 21);
        
        // Checksum and its validation
        std::uint32_t stored_checksum = read32(bytes , total_bytes - 4);
        std::vector<std::uint8_t>temp(bytes.begin() , bytes.end()-4);
        std::uint32_t calculated_checksum = calculate_crc32(temp);

        if(stored_checksum != calculated_checksum){
            return std::nullopt;
        }
        
        // Key length 
        std::uint32_t key_size = read32(bytes , 29);

        // Removing 4 for prefix length , then 37 for all other fields includeing value size and checksum
        // type - 1 , sequence_number - 8 , transaction_id - 8 , timestamp - 8 , key_length - 4 , value_length - 4 , checksum - 4 = 37 
        if(key_size > total_bytes - 4 - 37 ){
            return std::nullopt;
        }

        // Actual key
        std::string key = std::string(bytes.begin()+33 , bytes.begin()+33+key_size);

        // Value length
        std::uint32_t value_size = read32(bytes , 33+key_size);

        // Value length validation
        if(value_size != total_bytes - 4 - 37 - key_size){
            return std::nullopt;
        }

        // Actual value
        std::string value = std::string(bytes.begin()+37+key_size , bytes.begin()+37+key_size+value_size);


        return WalRecord {
            static_cast<RecordType>(type),
            sequence_number,
            transaction_id,
            timestamp,
            key,
            value
        };
    }
}
