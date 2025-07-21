cmake_minimum_required(VERSION 3.10)
project(BankingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(src/headers)

# Source files
set(SOURCES
    src/main.cpp
    src/implementation/Account.cpp
    src/implementation/Customer.cpp
    src/implementation/Transaction.cpp
    src/implementation/Loan.cpp
    src/implementation/Bank.cpp
    src/implementation/FileManager.cpp
    src/implementation/Login.cpp
    src/implementation/UI.cpp
    src/implementation/Utilities.cpp
)

# Create executable
add_executable(banking_system ${SOURCES})

# Compiler flags
target_compile_options(banking_system PRIVATE -Wall -Wextra)

# Create directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/backup)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)
