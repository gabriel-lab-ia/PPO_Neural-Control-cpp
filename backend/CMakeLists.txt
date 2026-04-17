cmake_minimum_required(VERSION 3.24)

project(OrbitalBackend LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system json)
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    find_library(SQLITE3_FALLBACK_LIBRARY sqlite3)
    if(NOT SQLITE3_FALLBACK_LIBRARY)
        message(FATAL_ERROR "SQLite3 library is required for orbital_backend")
    endif()
endif()

set(ORBITAL_BACKEND_SOURCES
    src/main.cpp
    src/common/http_utils.cpp
    src/common/logger.cpp
    src/common/time_utils.cpp
    src/persistence/sqlite_store.cpp
    src/telemetry/csv_telemetry_store.cpp
    src/replay/replay_service.cpp
    src/application/mission_service.cpp
    src/application/job_service.cpp
    src/transport/json_serialization.cpp
    src/transport/http_server.cpp
)

add_executable(orbital_backend ${ORBITAL_BACKEND_SOURCES})

target_compile_features(orbital_backend PRIVATE cxx_std_20)

target_include_directories(
    orbital_backend PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(SQLite3_FOUND)
    target_link_libraries(orbital_backend PRIVATE Boost::system Boost::json SQLite::SQLite3 pthread)
else()
    target_link_libraries(orbital_backend PRIVATE Boost::system Boost::json ${SQLITE3_FALLBACK_LIBRARY} pthread)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        orbital_backend PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wformat=2
    )
endif()
