#pragma once 

#include "wal_record.hpp"
#include "serializer.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <stdexcept>

namespace storage {
    class WalWriter{
      private:
        std::ofstream outFile;
        std::mutex mtx;
        bool failed = false;
      public:
        WalWriter(const std::string& filepath);
        void append(const WalRecord& record);
    };
}