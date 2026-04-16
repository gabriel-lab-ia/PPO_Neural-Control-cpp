cmake_minimum_required(VERSION 3.24)

project(
    OrbitalNeuralControlCPP
    VERSION 0.3.1
    DESCRIPTION "CPU-first C++20 orbital PPO baseline with reproducible train/eval/benchmark workflows"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Global language/tooling defaults
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
# Build options
# NMC_* prefix intentionally follows the stable runtime target name: nmc.
# -----------------------------------------------------------------------------
option(NMC_ENABLE_WARNINGS "Enable stricter compiler warnings" ON)
option(NMC_ENABLE_ASAN "Enable AddressSanitizer in Debug builds" OFF)
option(NMC_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer in Debug builds" OFF)
option(NMC_ENABLE_LTO "Enable link-time optimization in Release-like builds" OFF)
option(NMC_ENABLE_MUJOCO "Enable MuJoCo environment support" OFF)
option(NMC_BUILD_ORBITAL_CORE "Build orbital core/control/sim library + tests" ON)
option(NMC_BUILD_ORBITAL_BACKEND "Build orbital backend WebSocket/REST service" OFF)

# -----------------------------------------------------------------------------
# Dependency roots (cache + env override)
# -----------------------------------------------------------------------------
set(NMC_EIGEN_DEFAULT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/eigen-source")
set(NMC_TORCH_DEFAULT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/libtorch")

set(NMC_EIGEN_ROOT "${NMC_EIGEN_DEFAULT_ROOT}" CACHE PATH "Path to Eigen root")
set(NMC_TORCH_ROOT "${NMC_TORCH_DEFAULT_ROOT}" CACHE PATH "Path to LibTorch root")
set(NMC_MUJOCO_ROOT "" CACHE PATH "Optional MuJoCo installation root")

function(nmc_override_root_from_env cache_var default_value env_var probe_relative)
    if(
        DEFINED ENV{${env_var}}
        AND "${${cache_var}}" STREQUAL "${default_value}"
        AND EXISTS "$ENV{${env_var}}/${probe_relative}"
    )
        set(${cache_var} "$ENV{${env_var}}" CACHE PATH "Path to ${cache_var}" FORCE)
    endif()
endfunction()

nmc_override_root_from_env(NMC_EIGEN_ROOT "${NMC_EIGEN_DEFAULT_ROOT}" NMC_EIGEN_ROOT "Eigen/Dense")
nmc_override_root_from_env(NMC_TORCH_ROOT "${NMC_TORCH_DEFAULT_ROOT}" NMC_TORCH_ROOT "share/cmake/Torch/TorchConfig.cmake")

if(NOT EXISTS "${NMC_EIGEN_ROOT}/Eigen/Dense")
    message(FATAL_ERROR "Eigen not found at ${NMC_EIGEN_ROOT}")
endif()

if(NOT EXISTS "${NMC_TORCH_ROOT}/share/cmake/Torch/TorchConfig.cmake")
    message(FATAL_ERROR "LibTorch not found at ${NMC_TORCH_ROOT}")
endif()

set(NMC_EXPECTED_TORCH_DIR "${NMC_TORCH_ROOT}/share/cmake/Torch")
if(
    DEFINED Torch_DIR
    AND NOT "${Torch_DIR}" STREQUAL "${NMC_EXPECTED_TORCH_DIR}"
    AND EXISTS "${NMC_EXPECTED_TORCH_DIR}/TorchConfig.cmake"
)
    message(STATUS "Resetting cached Torch_DIR to match NMC_TORCH_ROOT")
    set(Torch_DIR "${NMC_EXPECTED_TORCH_DIR}" CACHE PATH "Torch package configuration directory" FORCE)
endif()

list(PREPEND CMAKE_PREFIX_PATH "${NMC_TORCH_ROOT}")
find_package(Torch REQUIRED CONFIG)

find_library(
    NMC_SQLITE3_LIBRARY
    NAMES sqlite3 libsqlite3.so.0
    HINTS /usr/lib /usr/lib/x86_64-linux-gnu /lib /lib/x86_64-linux-gnu
)
if(NOT NMC_SQLITE3_LIBRARY)
    message(FATAL_ERROR "SQLite3 runtime library not found")
endif()

# -----------------------------------------------------------------------------
# Compiler/linker profile helpers
# -----------------------------------------------------------------------------
function(nmc_apply_warning_profile target)
    if(NMC_ENABLE_WARNINGS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(
                ${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Wnull-dereference
                -Wdouble-promotion
                -Wformat=2
            )
        elseif(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive-)
        endif()
    endif()
endfunction()

function(nmc_apply_security_profile target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(${target} PRIVATE _FORTIFY_SOURCE=3)
        target_compile_options(
            ${target} PRIVATE
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            $<$<NOT:$<CONFIG:Debug>>:-fstack-protector-strong>
        )
        target_link_options(
            ${target} PRIVATE
            $<$<PLATFORM_ID:Linux>:-Wl,-z,relro,-z,now>
        )
    endif()
endfunction()

function(nmc_apply_sanitizers target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(NMC_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
        endif()
        if(NMC_ENABLE_UBSAN)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=undefined>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=undefined>)
        endif()
    endif()
endfunction()

function(nmc_try_enable_lto target)
    if(NMC_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT nmc_ipo_supported OUTPUT nmc_ipo_error)
        if(nmc_ipo_supported)
            set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "IPO/LTO unavailable: ${nmc_ipo_error}")
        endif()
    endif()
endfunction()

message(STATUS "Orbital Neural Control CPP build configuration")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  NMC_TORCH_ROOT: ${NMC_TORCH_ROOT}")
message(STATUS "  NMC_EIGEN_ROOT: ${NMC_EIGEN_ROOT}")
message(STATUS "  NMC_ENABLE_MUJOCO: ${NMC_ENABLE_MUJOCO}")
message(STATUS "  NMC_BUILD_ORBITAL_CORE: ${NMC_BUILD_ORBITAL_CORE}")
message(STATUS "  NMC_BUILD_ORBITAL_BACKEND: ${NMC_BUILD_ORBITAL_BACKEND}")
message(STATUS "  SQLite library: ${NMC_SQLITE3_LIBRARY}")
message(STATUS "  Primary CLI target: nmc")

# -----------------------------------------------------------------------------
# Baseline runtime target: nmc
# -----------------------------------------------------------------------------
set(NMC_BASELINE_SOURCES
    src/common/json_utils.cpp
    src/common/time_utils.cpp
    src/domain/config/experiment_config.cpp
    src/domain/env/environment_factory.cpp
    src/domain/env/point_mass_env.cpp
    src/domain/ppo/policy_value_model.cpp
    src/domain/ppo/trainer.cpp
    src/domain/inference/inference_backend_factory.cpp
    src/domain/inference/libtorch_policy_backend.cpp
    src/domain/inference/tensorrt_policy_backend_stub.cpp
    src/infrastructure/artifacts/artifact_layout.cpp
    src/infrastructure/artifacts/checkpoint_manager.cpp
    src/infrastructure/persistence/sqlite_experiment_store.cpp
    src/infrastructure/reporting/live_rollout_writer.cpp
    src/infrastructure/reporting/metrics_csv_writer.cpp
    src/application/benchmark_runner.cpp
    src/application/evaluation_runner.cpp
    src/application/training_runner.cpp
    src/interfaces/cli/command_line.cpp
    src/interfaces/cli/main.cpp
)

if(NMC_ENABLE_MUJOCO)
    if(NMC_MUJOCO_ROOT)
        set(_nmc_mujoco_hints "${NMC_MUJOCO_ROOT}")
    elseif(DEFINED ENV{MUJOCO_ROOT})
        set(_nmc_mujoco_hints "$ENV{MUJOCO_ROOT}")
    else()
        set(_nmc_mujoco_hints "")
    endif()

    find_path(
        MUJOCO_INCLUDE_DIR
        NAMES mujoco/mujoco.h
        HINTS ${_nmc_mujoco_hints}
        PATH_SUFFIXES include
    )
    find_library(
        MUJOCO_LIBRARY
        NAMES mujoco libmujoco
        HINTS ${_nmc_mujoco_hints}
        PATH_SUFFIXES lib bin
    )

    if(NOT MUJOCO_INCLUDE_DIR OR NOT MUJOCO_LIBRARY)
        message(FATAL_ERROR "MuJoCo requested but not found. Set NMC_MUJOCO_ROOT or MUJOCO_ROOT.")
    endif()

    list(APPEND NMC_BASELINE_SOURCES src/domain/env/mujoco_cartpole_env.cpp)
endif()

add_executable(nmc)
target_sources(nmc PRIVATE ${NMC_BASELINE_SOURCES})
target_compile_features(nmc PRIVATE cxx_std_20)

target_include_directories(
    nmc SYSTEM PRIVATE
    "${NMC_EIGEN_ROOT}"
    "${TORCH_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(nmc PRIVATE "${TORCH_LIBRARIES}" "${NMC_SQLITE3_LIBRARY}")

if(NMC_ENABLE_MUJOCO)
    target_include_directories(nmc SYSTEM PRIVATE "${MUJOCO_INCLUDE_DIR}")
    target_link_libraries(nmc PRIVATE "${MUJOCO_LIBRARY}")
    target_compile_definitions(nmc PRIVATE NMC_ENABLE_MUJOCO=1)

    get_filename_component(MUJOCO_LIBRARY_DIR "${MUJOCO_LIBRARY}" DIRECTORY)
    set_target_properties(
        nmc
        PROPERTIES
            BUILD_RPATH "${MUJOCO_LIBRARY_DIR}"
            INSTALL_RPATH "${MUJOCO_LIBRARY_DIR}"
    )
endif()

set_target_properties(
    nmc
    PROPERTIES
        OUTPUT_NAME "nmc"
        POSITION_INDEPENDENT_CODE ON
)

if(TORCH_CXX_FLAGS)
    separate_arguments(TORCH_CXX_FLAGS_LIST NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    target_compile_options(nmc PRIVATE ${TORCH_CXX_FLAGS_LIST})
endif()

nmc_apply_warning_profile(nmc)
nmc_apply_security_profile(nmc)
nmc_apply_sanitizers(nmc)
nmc_try_enable_lto(nmc)

# -----------------------------------------------------------------------------
# Baseline smoke test contract
# -----------------------------------------------------------------------------
enable_testing()
add_test(
    NAME nmc_smoke_benchmark
    COMMAND nmc benchmark --quick --name ctest_smoke --seed 7
)
set_tests_properties(
    nmc_smoke_benchmark
    PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TIMEOUT 240
)

# -----------------------------------------------------------------------------
# Optional expansion modules
# -----------------------------------------------------------------------------
if(NMC_BUILD_ORBITAL_CORE)
    add_subdirectory(core)
endif()

if(NMC_BUILD_ORBITAL_BACKEND)
    find_package(Boost QUIET COMPONENTS system)
    if(Boost_FOUND)
        add_subdirectory(backend)
    else()
        message(WARNING "Boost::system not found. Skipping orbital backend target. Install libboost-all-dev and reconfigure to enable it.")
    endif()
endif()
