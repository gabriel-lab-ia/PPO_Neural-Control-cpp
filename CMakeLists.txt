cmake_minimum_required(VERSION 3.24)

project(
    NeuroMotorCPP
    VERSION 0.1.0
    DESCRIPTION "Local C++20 sandbox for private ML/RL experiments with Eigen and LibTorch"
    LANGUAGES CXX
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(NMC_ENABLE_WARNINGS "Enable stricter compiler warnings" ON)
option(NMC_ENABLE_ASAN "Enable AddressSanitizer in Debug builds" OFF)
option(NMC_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer in Debug builds" OFF)
option(NMC_ENABLE_LTO "Enable link-time optimization in Release-like builds" OFF)
option(NMC_ENABLE_MUJOCO "Enable MuJoCo environment support" OFF)

set(NMC_EIGEN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/eigen-source" CACHE PATH "Path to Eigen root")
set(NMC_TORCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/libtorch" CACHE PATH "Path to LibTorch root")
set(NMC_MUJOCO_ROOT "" CACHE PATH "Optional MuJoCo installation root")

if(DEFINED ENV{NMC_EIGEN_ROOT} AND NOT EXISTS "${NMC_EIGEN_ROOT}/Eigen/Dense")
    set(NMC_EIGEN_ROOT "$ENV{NMC_EIGEN_ROOT}" CACHE PATH "Path to Eigen root" FORCE)
endif()

if(DEFINED ENV{NMC_TORCH_ROOT} AND NOT EXISTS "${NMC_TORCH_ROOT}/share/cmake/Torch/TorchConfig.cmake")
    set(NMC_TORCH_ROOT "$ENV{NMC_TORCH_ROOT}" CACHE PATH "Path to LibTorch root" FORCE)
endif()

if(NOT EXISTS "${NMC_EIGEN_ROOT}/Eigen/Dense")
    message(FATAL_ERROR "Eigen not found at ${NMC_EIGEN_ROOT}.")
endif()

if(NOT EXISTS "${NMC_TORCH_ROOT}/share/cmake/Torch/TorchConfig.cmake")
    message(FATAL_ERROR "LibTorch not found at ${NMC_TORCH_ROOT}.")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${NMC_TORCH_ROOT}")
find_package(Torch REQUIRED)

set(NMC_MOTOR_SOURCES
    src/main.cpp
    src/app/training_app.cpp
    src/env/environment_registry.cpp
    src/env/point_mass_env.cpp
    src/model/ppo_agent.cpp
    src/train/ppo_trainer.cpp
    src/utils/csv_logger.cpp
    src/utils/live_rollout_logger.cpp
    src/utils/neural_3d_export.cpp
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

    list(APPEND NMC_MOTOR_SOURCES src/env/mujoco_cartpole_env.cpp)
endif()

add_executable(motor ${NMC_MOTOR_SOURCES})
target_compile_features(motor PRIVATE cxx_std_20)
target_include_directories(
    motor SYSTEM PRIVATE
    "${NMC_EIGEN_ROOT}"
    "${TORCH_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(motor PRIVATE "${TORCH_LIBRARIES}")

if(NMC_ENABLE_MUJOCO)
    target_include_directories(motor SYSTEM PRIVATE "${MUJOCO_INCLUDE_DIR}")
    target_link_libraries(motor PRIVATE "${MUJOCO_LIBRARY}")
    target_compile_definitions(motor PRIVATE NMC_ENABLE_MUJOCO=1)
    get_filename_component(MUJOCO_LIBRARY_DIR "${MUJOCO_LIBRARY}" DIRECTORY)
    set_target_properties(
        motor
        PROPERTIES
            BUILD_RPATH "${MUJOCO_LIBRARY_DIR}"
            INSTALL_RPATH "${MUJOCO_LIBRARY_DIR}"
    )
endif()

set_target_properties(
    motor
    PROPERTIES
        OUTPUT_NAME "motor"
        POSITION_INDEPENDENT_CODE ON
)

if(TORCH_CXX_FLAGS)
    separate_arguments(TORCH_CXX_FLAGS_LIST NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    target_compile_options(motor PRIVATE ${TORCH_CXX_FLAGS_LIST})
endif()

if(NMC_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            motor PRIVATE
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
        target_compile_options(motor PRIVATE /W4 /permissive-)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(motor PRIVATE _FORTIFY_SOURCE=3)
    target_compile_options(
        motor PRIVATE
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<NOT:$<CONFIG:Debug>>:-fstack-protector-strong>
    )
    target_link_options(
        motor PRIVATE
        $<$<PLATFORM_ID:Linux>:-Wl,-z,relro,-z,now>
    )
endif()

if(NMC_ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(motor PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
    target_link_options(motor PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
endif()

if(NMC_ENABLE_UBSAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(motor PRIVATE $<$<CONFIG:Debug>:-fsanitize=undefined>)
    target_link_options(motor PRIVATE $<$<CONFIG:Debug>:-fsanitize=undefined>)
endif()

if(NMC_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT nmc_ipo_supported OUTPUT nmc_ipo_error)
    if(nmc_ipo_supported)
        set_property(TARGET motor PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO/LTO unavailable: ${nmc_ipo_error}")
    endif()
endif()
