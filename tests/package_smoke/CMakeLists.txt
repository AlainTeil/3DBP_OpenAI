cmake_minimum_required(VERSION 3.20)

project(three_dbp_package_smoke LANGUAGES CXX)

find_package(three_dbp CONFIG REQUIRED)

add_executable(three_dbp_package_smoke smoke.cpp)
target_link_libraries(three_dbp_package_smoke PRIVATE three_dbp::three_dbp)
target_compile_features(three_dbp_package_smoke PRIVATE cxx_std_20)
