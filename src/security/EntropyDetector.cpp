#include "security/EntropyDetector.h"
#include <array>
#include <cmath>
#include <stdexcept>

EntropyDetector::EntropyDetector(
    double threshold,
    std::size_t minimumValueLength
) : threshold(threshold),
    minimumValueLength(minimumValueLength) {

    if (threshold <= 0.0 || threshold > 8.0) {
        throw std::invalid_argument(
            "Entropy threshold must be in (0, 8]"
        );
    }

    if (minimumValueLength == 0) {
        throw std::invalid_argument(
            "Minimum value length must be positive"
        );
    }
}

DetectionResult EntropyDetector::analyze(
    const oops::Request& request
) {
    DetectionResult result{};
    result.Detector = "EntropyDetector";
    result.Score = 0;
    result.Suspicious_flag = false;
    result.reason = "Normal activity";
    result.HowSevere = Severity::LOW;

    if (request.command != oops::Command::Set) {
        return result;
    }

    const std::string& value = request.value;

    if (value.size() < minimumValueLength) {
        result.reason = "Value too short for entropy analysis";
        return result;
    }

    std::array<std::size_t, 256> frequencies{};

    for (unsigned char character : value) {
        ++frequencies[character];
    }

    double entropy = 0.0;
    const double length = static_cast<double>(value.size());

    for (std::size_t frequency : frequencies) {
        if (frequency == 0) {
            continue;
        }

        const double probability =
            static_cast<double>(frequency) / length;

        entropy -= probability * std::log2(probability);
    }

    if (entropy >= threshold) {
        result.Suspicious_flag = true;
        result.Score = 60;
        result.reason = "High-entropy value detected";
        result.HowSevere = Severity::MEDIUM;
    }

    return result;
}