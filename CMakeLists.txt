cmake_minimum_required(VERSION 3.20)

project(three_dbp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(THREEDBP_BUILD_TESTS "Build tests" ON)
option(THREEDBP_BUILD_DOCS "Build documentation" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

# Dependencies
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.0.2
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt nlohmann_json)

# Library
add_library(three_dbp
    src/model.cpp
    src/validation.cpp
    src/instance_io.cpp
    src/packer.cpp
    src/algorithms/common.cpp
    src/algorithms/ffd.cpp
    src/algorithms/nfdh.cpp
    src/algorithms/guillotine.cpp
    src/algorithms/layered.cpp
    src/algorithms/maximal_space.cpp
    src/algorithms/metaheuristics.cpp
    src/algorithms/online_ffd.cpp
)

add_library(three_dbp::three_dbp ALIAS three_dbp)

target_include_directories(three_dbp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(three_dbp PRIVATE $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>)

target_compile_features(three_dbp PUBLIC cxx_std_20)

function(threedbp_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()

threedbp_enable_warnings(three_dbp)

# CLI executables
add_executable(three_dbp_pack apps/pack.cpp)
target_link_libraries(three_dbp_pack PRIVATE three_dbp fmt::fmt)
threedbp_enable_warnings(three_dbp_pack)

add_executable(three_dbp_generate apps/generate.cpp)
target_link_libraries(three_dbp_generate PRIVATE three_dbp)
threedbp_enable_warnings(three_dbp_generate)

# Tests
if(THREEDBP_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(three_dbp_tests
        tests/model_tests.cpp
        tests/packer_tests.cpp
        tests/instance_io_tests.cpp
        tests/adversarial_tests.cpp
        tests/integration_tests.cpp
        tests/system_tests.cpp
        tests/cli_golden_tests.cpp
        tests/property_tests.cpp
        tests/edge_case_tests.cpp)
    target_link_libraries(three_dbp_tests PRIVATE three_dbp GTest::gtest_main GTest::gmock)
    target_compile_definitions(three_dbp_tests PRIVATE
        THREEDBP_GENERATE_EXE="$<TARGET_FILE:three_dbp_generate>"
        THREEDBP_PACK_EXE="$<TARGET_FILE:three_dbp_pack>")
    add_dependencies(three_dbp_tests three_dbp_generate three_dbp_pack)
    threedbp_enable_warnings(three_dbp_tests)

    include(GoogleTest)
    gtest_discover_tests(three_dbp_tests)
endif()

install(TARGETS three_dbp
    EXPORT three_dbpTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS three_dbp_pack three_dbp_generate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/three_dbpConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/three_dbpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/three_dbpConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/three_dbp)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/three_dbpConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/three_dbpConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/three_dbp)

install(EXPORT three_dbpTargets
    NAMESPACE three_dbp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/three_dbp)

# Docs
if(THREEDBP_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src")
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    endif()
endif()
