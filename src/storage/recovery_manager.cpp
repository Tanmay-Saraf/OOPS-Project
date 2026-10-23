#include "recovery_manager.hpp"
#include "serializer.hpp"

#include <exception>
#include <utility>

namespace storage{

    RecoveryReport RecoveryManager::recover(const std::string& wal_path,const ReplayHandler& replay,std::optional<std::uint64_t> target_timestamp_us) const {

        RecoveryReport report;
        WalReadResult wal;

        try{
            WalReader reader(wal_path);
            wal = reader.read_all();
        }catch(const std::exception&){
            report.status = WalReadStatus::IO_ERROR;
            report.error_offset = 0;
            return report;
        }

        report.status = wal.status;
        report.error_offset = wal.error_offset;

        if(wal.status == WalReadStatus::IO_ERROR){
            return report;
        }

        if(!replay){
            report.replay_failed = true;
            report.error_offset = 0;
            return report;
        }

        std::optional<std::uint64_t> previous_sequence;
        std::uint64_t offset = 0;

        for(const WalRecord& record : wal.records){

            if(record.sequence_number == 0 ||(previous_sequence && record.sequence_number <= *previous_sequence)){
                report.status = WalReadStatus::CORRUPTED;
                report.error_offset = offset;
                break;
            }

            switch(record.type){
                case RecordType::PUT:
                case RecordType::DELETE:
                    break;

                default:
                    report.status = WalReadStatus::CORRUPTED;
                    report.error_offset = offset;
                    return report;
            }

            previous_sequence = record.sequence_number;

            const bool within_target_time =!target_timestamp_us || record.timestamp <= *target_timestamp_us;

            if(within_target_time){
                if(!replay(record)){
                    report.replay_failed = true;
                    report.error_offset = offset;
                    break;
                }

                report.records_restored += 1;
            }

            offset += static_cast<std::uint64_t>(serialize(record).size());
        }

        return report;
    }

}