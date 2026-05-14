#pragma once

#include <string>

#include "bp/model.hpp"

namespace bp {

Instance load_instance(const std::string &path);

PackingResult load_result(const std::string &path);

void save_instance(const Instance &instance, const std::string &path);

void save_result(const PackingResult &result, const std::string &path);

} // namespace bp
