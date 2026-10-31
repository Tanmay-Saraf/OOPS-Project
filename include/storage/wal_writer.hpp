#pragma once 

#include "wal_record.hpp"
#include "serializer.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <stdexcept>

namespace storage {
    class WalWriter{
      private:
        int fd = -1;
        std::mutex mtx;
        bool failed = false;
      public:
        explicit WalWriter(const std::string& filepath);
        ~WalWriter();
        void append(const WalRecord& record);
    };
}