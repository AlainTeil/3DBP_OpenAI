#pragma once

#include <gtest/gtest.h>

#include "bp/validation.hpp"

inline void expect_valid_packing(const bp::Instance &instance, const bp::PackingResult &result) {
    const auto report = bp::validate_result(instance, result);
    EXPECT_TRUE(report.valid) << [&]() {
        std::string message;
        for (const auto &error : report.errors) {
            message += error + "\n";
        }
        return message;
    }();
    EXPECT_TRUE(result.validation_errors.empty()) << [&]() {
        std::string message;
        for (const auto &error : result.validation_errors) {
            message += error + "\n";
        }
        return message;
    }();
}
