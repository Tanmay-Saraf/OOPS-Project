#pragma once 

#include "wal_record.hpp"
#include "serializer.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace storage {
  class RecoveryManager;
    class WalWriter {
      private:
        std::string filepath;
        int fd = -1;
        std::mutex mtx;
        bool failed = false;

        void replace_records_locked(const std::vector<WalRecord>& records);

      public:
        explicit WalWriter(const std::string& filepath);
        ~WalWriter();
        void append(const WalRecord& record);

        friend class RecoveryManager;
    };
}