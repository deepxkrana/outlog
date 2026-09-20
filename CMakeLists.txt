cmake_minimum_required(VERSION 3.14)
project(outlog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# SQLite3 is required
find_package(SQLite3 REQUIRED)

# Fetch CLI11 for argument parsing
include(FetchContent)
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.1
)
FetchContent_MakeAvailable(cli11)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/cli.cpp
    src/database.cpp
    src/process_manager.cpp
    src/pty_session.cpp
)

# Executable
add_executable(outlog ${SOURCES})

# Link SQLite3 and CLI11
target_link_libraries(outlog PRIVATE SQLite3::SQLite3 CLI11::CLI11)

# Installation targets for Debian packaging
include(GNUInstallDirs)
install(TARGETS outlog DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES scripts/bash-integration.sh DESTINATION ${CMAKE_INSTALL_DATADIR}/outlog)
install(FILES scripts/outlog.sh DESTINATION /etc/profile.d)

# Testing
enable_testing()
add_executable(test_database tests/test_database.cpp src/database.cpp)
target_link_libraries(test_database PRIVATE SQLite3::SQLite3)
add_test(NAME DatabaseTest COMMAND test_database)
