#include <iostream>

#include "bp/packer.hpp"
#include "bp/validation.hpp"

int main() {
    bp::Instance instance;
    instance.bins.push_back(bp::Bin{.id = "bin-0", .size = bp::Vec3{10, 10, 10}});
    instance.boxes.push_back(bp::Box{.id = "box-0", .size = bp::Vec3{5, 5, 5}});
    instance.boxes.push_back(bp::Box{.id = "box-1", .size = bp::Vec3{5, 5, 5}});

    const auto result = bp::pack(instance, bp::PackOptions{bp::AlgorithmId::FFD, 0u, 10});
    const auto validation = bp::validate_result(instance, result);
    if (result.status != bp::PackingStatus::Feasible || !validation.valid) {
        std::cerr << "Installed package smoke test failed\n";
        for (const auto &error : validation.errors) {
            std::cerr << error << '\n';
        }
        return 1;
    }

    return 0;
}
