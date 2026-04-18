cmake_minimum_required(VERSION 3.24)

project(
    OrbitalNeuralControlCPP
    VERSION 0.4.1
    DESCRIPTION "CPU-first C++20 orbital PPO baseline with reproducible train/eval/benchmark workflows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(NMC_ENABLE_WARNINGS "Enable stricter compiler warnings" ON)
option(NMC_ENABLE_ASAN "Enable AddressSanitizer in Debug builds" OFF)
option(NMC_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer in Debug builds" OFF)
option(NMC_ENABLE_TSAN "Enable ThreadSanitizer in Debug builds" OFF)
option(NMC_ENABLE_COVERAGE "Enable gcov/llvm-cov instrumentation" OFF)
option(NMC_ENABLE_LTO "Enable link-time optimization in Release-like builds" OFF)
set(NMC_PGO_MODE "OFF" CACHE STRING "Profile-guided optimization mode: OFF|GENERATE|USE|AUTO")
set_property(CACHE NMC_PGO_MODE PROPERTY STRINGS OFF GENERATE USE AUTO)
option(NMC_USE_LEGACY_VENDOR_DEPS "Allow fallback to vendored dependencies in lib/" OFF)
option(NMC_ENABLE_MUJOCO "Enable MuJoCo environment support" OFF)
option(NMC_ENABLE_TENSORRT "Enable optional TensorRT runtime hooks (requires TensorRT SDK)" OFF)
option(NMC_BUILD_ORBITAL_CORE "Build orbital core/control/sim library + tests" OFF)
option(NMC_BUILD_ORBITAL_BACKEND "Build orbital backend WebSocket/REST service" OFF)

# -----------------------------------------------------------------------------
# Dependencies (vcpkg-first, optional legacy fallback)
# -----------------------------------------------------------------------------
set(NMC_EIGEN_DEFAULT_ROOT "")
set(NMC_TORCH_DEFAULT_ROOT "")
set(NMC_EIGEN_ROOT "${NMC_EIGEN_DEFAULT_ROOT}" CACHE PATH "Legacy Eigen root when fallback is enabled")
set(NMC_TORCH_ROOT "${NMC_TORCH_DEFAULT_ROOT}" CACHE PATH "Legacy LibTorch root when fallback is enabled")
set(NMC_MUJOCO_ROOT "" CACHE PATH "Optional MuJoCo installation root")
set(NMC_TENSORRT_ROOT "" CACHE PATH "Optional TensorRT SDK root")

if(NOT NMC_USE_LEGACY_VENDOR_DEPS)
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "^/scripts/buildsystems/vcpkg\\.cmake$")
        message(
            FATAL_ERROR
            "VCPKG_ROOT is not set. Run ./tools/setup_vcpkg.sh and export VCPKG_ROOT before configuring."
        )
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE AND NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(
            FATAL_ERROR
            "Configured CMAKE_TOOLCHAIN_FILE does not exist: ${CMAKE_TOOLCHAIN_FILE}. "
            "Ensure VCPKG_ROOT points to a valid vcpkg checkout."
        )
    endif()
endif()

find_package(Eigen3 CONFIG QUIET)
if(NOT Eigen3_FOUND)
    if(NMC_USE_LEGACY_VENDOR_DEPS AND EXISTS "${NMC_EIGEN_ROOT}/Eigen/Dense")
        set(NMC_EIGEN_INCLUDE_DIR "${NMC_EIGEN_ROOT}")
    else()
        message(
            FATAL_ERROR
            "Eigen3 not found. Configure with vcpkg toolchain or enable NMC_USE_LEGACY_VENDOR_DEPS and provide NMC_EIGEN_ROOT."
        )
    endif()
endif()

find_package(Torch CONFIG QUIET)
if(NOT Torch_FOUND)
    if(NMC_USE_LEGACY_VENDOR_DEPS AND EXISTS "${NMC_TORCH_ROOT}/share/cmake/Torch/TorchConfig.cmake")
        list(PREPEND CMAKE_PREFIX_PATH "${NMC_TORCH_ROOT}")
        find_package(Torch REQUIRED CONFIG)
    else()
        message(
            FATAL_ERROR
            "LibTorch not found. Use vcpkg (recommended) or enable NMC_USE_LEGACY_VENDOR_DEPS and set NMC_TORCH_ROOT."
        )
    endif()
endif()

find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    set(NMC_SQLITE_LINK_TARGET SQLite::SQLite3)
else()
    find_library(
        NMC_SQLITE3_LIBRARY
        NAMES sqlite3 libsqlite3.so.0
        HINTS /usr/lib /usr/lib/x86_64-linux-gnu /lib /lib/x86_64-linux-gnu
    )
    if(NOT NMC_SQLITE3_LIBRARY)
        message(FATAL_ERROR "SQLite3 runtime library not found")
    endif()
    set(NMC_SQLITE_LINK_TARGET "${NMC_SQLITE3_LIBRARY}")
endif()

set(NMC_WITH_TENSORRT OFF)
if(NMC_ENABLE_TENSORRT)
    set(_nmc_tensorrt_hints)
    if(NMC_TENSORRT_ROOT)
        list(APPEND _nmc_tensorrt_hints "${NMC_TENSORRT_ROOT}")
    endif()
    if(DEFINED ENV{TENSORRT_ROOT})
        list(APPEND _nmc_tensorrt_hints "$ENV{TENSORRT_ROOT}")
    endif()
    list(APPEND _nmc_tensorrt_hints /usr /usr/local)

    find_path(
        NMC_TENSORRT_INCLUDE_DIR
        NAMES NvInfer.h
        HINTS ${_nmc_tensorrt_hints}
        PATH_SUFFIXES include include/x86_64-linux-gnu
    )
    find_library(
        NMC_TENSORRT_NVINFER_LIBRARY
        NAMES nvinfer
        HINTS ${_nmc_tensorrt_hints}
        PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu
    )
    find_library(
        NMC_TENSORRT_PARSER_LIBRARY
        NAMES nvonnxparser
        HINTS ${_nmc_tensorrt_hints}
        PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu
    )
    find_library(
        NMC_TENSORRT_PLUGIN_LIBRARY
        NAMES nvinfer_plugin
        HINTS ${_nmc_tensorrt_hints}
        PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu
    )

    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        set(NMC_TENSORRT_CUDA_TARGET CUDA::cudart)
    else()
        find_library(
            NMC_TENSORRT_CUDART_LIBRARY
            NAMES cudart
            HINTS ${_nmc_tensorrt_hints}
            PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu targets/x86_64-linux/lib
        )
    endif()

    if(NOT NMC_TENSORRT_INCLUDE_DIR OR NOT NMC_TENSORRT_NVINFER_LIBRARY OR NOT NMC_TENSORRT_PARSER_LIBRARY)
        message(
            FATAL_ERROR
            "NMC_ENABLE_TENSORRT=ON, but TensorRT SDK components were not found (NvInfer + nvonnxparser). "
            "Set NMC_TENSORRT_ROOT/TENSORRT_ROOT to your SDK path."
        )
    endif()
    if(NOT CUDAToolkit_FOUND AND NOT NMC_TENSORRT_CUDART_LIBRARY)
        message(
            FATAL_ERROR
            "NMC_ENABLE_TENSORRT=ON, but CUDA runtime (cudart) was not found. "
            "Install CUDA toolkit or set NMC_TENSORRT_ROOT/TENSORRT_ROOT with a valid cudart library."
        )
    endif()

    set(NMC_WITH_TENSORRT ON)
endif()

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
        if(NMC_ENABLE_TSAN)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=thread>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=thread>)
        endif()
    endif()
endfunction()

function(nmc_apply_coverage target)
    if(NMC_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -O0 -g --coverage)
        target_link_options(${target} PRIVATE --coverage)
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

function(nmc_apply_pgo target)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        return()
    endif()

    string(TOUPPER "${NMC_PGO_MODE}" NMC_PGO_MODE_NORMALIZED)
    set(NMC_PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo")
    file(MAKE_DIRECTORY "${NMC_PGO_DATA_DIR}")
    set(NMC_PGO_PROFILE_PATH "${NMC_PGO_DATA_DIR}/default.profdata")

    if(NMC_PGO_MODE_NORMALIZED STREQUAL "GENERATE")
        target_compile_options(${target} PRIVATE -fprofile-generate=${NMC_PGO_DATA_DIR})
        target_link_options(${target} PRIVATE -fprofile-generate=${NMC_PGO_DATA_DIR})
    elseif(NMC_PGO_MODE_NORMALIZED STREQUAL "USE")
        target_compile_options(${target} PRIVATE -fprofile-use=${NMC_PGO_PROFILE_PATH} -fprofile-correction)
        target_link_options(${target} PRIVATE -fprofile-use=${NMC_PGO_PROFILE_PATH})
    elseif(NMC_PGO_MODE_NORMALIZED STREQUAL "AUTO")
        if(EXISTS "${NMC_PGO_PROFILE_PATH}")
            target_compile_options(${target} PRIVATE -fprofile-use=${NMC_PGO_PROFILE_PATH} -fprofile-correction)
            target_link_options(${target} PRIVATE -fprofile-use=${NMC_PGO_PROFILE_PATH})
        else()
            target_compile_options(${target} PRIVATE -fprofile-generate=${NMC_PGO_DATA_DIR})
            target_link_options(${target} PRIVATE -fprofile-generate=${NMC_PGO_DATA_DIR})
        endif()
    endif()
endfunction()

message(STATUS "Orbital Neural Control CPP build configuration")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  NMC_USE_LEGACY_VENDOR_DEPS: ${NMC_USE_LEGACY_VENDOR_DEPS}")
if(Torch_FOUND)
    message(STATUS "  Torch: found via package config")
else()
    message(STATUS "  NMC_TORCH_ROOT: ${NMC_TORCH_ROOT}")
endif()
if(Eigen3_FOUND)
    message(STATUS "  Eigen3: found via package config")
else()
    message(STATUS "  NMC_EIGEN_ROOT: ${NMC_EIGEN_ROOT}")
endif()
message(STATUS "  NMC_ENABLE_MUJOCO: ${NMC_ENABLE_MUJOCO}")
message(STATUS "  NMC_ENABLE_TENSORRT: ${NMC_ENABLE_TENSORRT}")
if(NMC_WITH_TENSORRT)
    message(STATUS "  TensorRT include: ${NMC_TENSORRT_INCLUDE_DIR}")
    message(STATUS "  TensorRT nvinfer: ${NMC_TENSORRT_NVINFER_LIBRARY}")
    message(STATUS "  TensorRT parser: ${NMC_TENSORRT_PARSER_LIBRARY}")
    if(NMC_TENSORRT_PLUGIN_LIBRARY)
        message(STATUS "  TensorRT plugin: ${NMC_TENSORRT_PLUGIN_LIBRARY}")
    endif()
    if(CUDAToolkit_FOUND)
        message(STATUS "  TensorRT CUDA runtime target: CUDA::cudart")
    else()
        message(STATUS "  TensorRT CUDA runtime: ${NMC_TENSORRT_CUDART_LIBRARY}")
    endif()
endif()
message(STATUS "  NMC_BUILD_ORBITAL_CORE: ${NMC_BUILD_ORBITAL_CORE}")
message(STATUS "  NMC_BUILD_ORBITAL_BACKEND: ${NMC_BUILD_ORBITAL_BACKEND}")
message(STATUS "  NMC_ENABLE_LTO: ${NMC_ENABLE_LTO}")
message(STATUS "  NMC_PGO_MODE: ${NMC_PGO_MODE}")
if(SQLite3_FOUND)
    message(STATUS "  SQLite3 version: ${SQLite3_VERSION}")
else()
    message(STATUS "  SQLite3 library: ${NMC_SQLITE3_LIBRARY}")
endif()
message(STATUS "  Primary CLI target: nmc")

set(NMC_RUNTIME_SOURCES
    src/common/determinism.cpp
    src/common/json_utils.cpp
    src/common/time_utils.cpp
    src/domain/config/experiment_config.cpp
    src/domain/config/config_validation.cpp
    src/domain/env/environment_factory.cpp
    src/domain/env/orbital_6dof_env.cpp
    src/domain/env/point_mass_env.cpp
    src/domain/ppo/gaussian_policy.cpp
    src/domain/ppo/policy_value_model.cpp
    src/domain/ppo/rollout_buffer.cpp
    src/domain/ppo/trainer.cpp
    src/domain/ppo/value_network.cpp
    src/domain/inference/inference_backend_factory.cpp
    src/domain/inference/inference_benchmark.cpp
    src/domain/inference/libtorch_policy_backend.cpp
    src/domain/inference/tensorrt_native_backend.cpp
    src/domain/inference/tensorrt_policy_backend.cpp
    src/domain/inference/tensorrt_policy_backend_stub.cpp
    src/infrastructure/artifacts/artifact_layout.cpp
    src/infrastructure/artifacts/checkpoint_manager.cpp
    src/infrastructure/artifacts/run_manifest.cpp
    src/infrastructure/persistence/sqlite_experiment_store.cpp
    src/infrastructure/reporting/live_rollout_writer.cpp
    src/infrastructure/reporting/metrics_csv_writer.cpp
    src/application/benchmark_runner.cpp
    src/application/evaluation_runner.cpp
    src/application/training_runner.cpp
    src/interfaces/cli/command_line.cpp
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

    list(APPEND NMC_RUNTIME_SOURCES src/domain/env/mujoco_cartpole_env.cpp)
endif()

add_library(nmc_runtime STATIC)
target_sources(nmc_runtime PRIVATE ${NMC_RUNTIME_SOURCES})
target_compile_features(nmc_runtime PUBLIC cxx_std_20)

target_include_directories(
    nmc_runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/rl/include"
)

if(DEFINED NMC_EIGEN_INCLUDE_DIR)
    target_include_directories(nmc_runtime SYSTEM PRIVATE "${NMC_EIGEN_INCLUDE_DIR}")
endif()
if(TORCH_INCLUDE_DIRS)
    target_include_directories(nmc_runtime SYSTEM PRIVATE "${TORCH_INCLUDE_DIRS}")
endif()

if(TARGET Torch::Torch)
    target_link_libraries(nmc_runtime PUBLIC Torch::Torch "${NMC_SQLITE_LINK_TARGET}")
else()
    target_link_libraries(nmc_runtime PUBLIC "${TORCH_LIBRARIES}" "${NMC_SQLITE_LINK_TARGET}")
endif()
if(Eigen3_FOUND)
    target_link_libraries(nmc_runtime PUBLIC Eigen3::Eigen)
endif()
if(NMC_WITH_TENSORRT)
    target_include_directories(nmc_runtime SYSTEM PRIVATE "${NMC_TENSORRT_INCLUDE_DIR}")
    target_link_libraries(
        nmc_runtime PUBLIC
        "${NMC_TENSORRT_NVINFER_LIBRARY}"
        "${NMC_TENSORRT_PARSER_LIBRARY}"
    )
    if(NMC_TENSORRT_PLUGIN_LIBRARY)
        target_link_libraries(nmc_runtime PUBLIC "${NMC_TENSORRT_PLUGIN_LIBRARY}")
    endif()
    if(CUDAToolkit_FOUND)
        target_link_libraries(nmc_runtime PUBLIC CUDA::cudart)
    else()
        target_link_libraries(nmc_runtime PUBLIC "${NMC_TENSORRT_CUDART_LIBRARY}")
    endif()
    target_compile_definitions(nmc_runtime PRIVATE NMC_WITH_TENSORRT=1)
endif()

if(TORCH_CXX_FLAGS)
    separate_arguments(TORCH_CXX_FLAGS_LIST NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    target_compile_options(nmc_runtime PRIVATE ${TORCH_CXX_FLAGS_LIST})
endif()

nmc_apply_warning_profile(nmc_runtime)
nmc_apply_security_profile(nmc_runtime)
nmc_apply_sanitizers(nmc_runtime)
nmc_apply_coverage(nmc_runtime)
nmc_apply_pgo(nmc_runtime)
nmc_try_enable_lto(nmc_runtime)

add_executable(nmc)
target_sources(nmc PRIVATE src/interfaces/cli/main.cpp)
target_compile_features(nmc PRIVATE cxx_std_20)
target_include_directories(nmc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(nmc PRIVATE nmc_runtime)

if(NMC_ENABLE_MUJOCO)
    target_include_directories(nmc_runtime SYSTEM PRIVATE "${MUJOCO_INCLUDE_DIR}")
    target_link_libraries(nmc_runtime PRIVATE "${MUJOCO_LIBRARY}")
    target_compile_definitions(nmc_runtime PRIVATE NMC_ENABLE_MUJOCO=1)

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

nmc_apply_warning_profile(nmc)
nmc_apply_security_profile(nmc)
nmc_apply_sanitizers(nmc)
nmc_apply_coverage(nmc)
nmc_apply_pgo(nmc)
nmc_try_enable_lto(nmc)

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

add_executable(nmc_inference_parity_test)
target_sources(nmc_inference_parity_test PRIVATE src/tests/inference_backend_parity.cpp)
target_compile_features(nmc_inference_parity_test PRIVATE cxx_std_20)
target_include_directories(
    nmc_inference_parity_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(nmc_inference_parity_test PRIVATE nmc_runtime)
nmc_apply_warning_profile(nmc_inference_parity_test)
nmc_apply_security_profile(nmc_inference_parity_test)
nmc_apply_sanitizers(nmc_inference_parity_test)
nmc_apply_coverage(nmc_inference_parity_test)
nmc_apply_pgo(nmc_inference_parity_test)
nmc_try_enable_lto(nmc_inference_parity_test)

add_test(
    NAME nmc_inference_parity
    COMMAND nmc_inference_parity_test
)
set_tests_properties(
    nmc_inference_parity
    PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TIMEOUT 120
)

if(NMC_BUILD_ORBITAL_CORE)
    add_subdirectory(core)
endif()

if(NMC_BUILD_ORBITAL_BACKEND)
    find_package(Boost QUIET COMPONENTS system json)
    if(Boost_FOUND)
        add_subdirectory(backend)
    else()
        message(WARNING "Boost::system/json not found. Skipping orbital backend target. Install libboost-all-dev and reconfigure to enable it.")
    endif()
endif()
