#ifndef ENTROPY_DETECTOR_H
#define ENTROPY_DETECTOR_H

#include "SecurityDetector.h"
#include <cstddef>

class EntropyDetector : public SecurityDetector {
private:
    const double threshold;
    const std::size_t minimumValueLength;

public:
    explicit EntropyDetector(
        double threshold = 4.2,
        std::size_t minimumValueLength = 16
    );

    DetectionResult analyze(const oops::Request& request) override;
};

#endif