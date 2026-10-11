cmake_minimum_required(VERSION 3.10)
project(BankingSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core library shared by the application and the tests
add_library(banking_core
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
target_include_directories(banking_core PUBLIC src/headers)
target_compile_options(banking_core PRIVATE -Wall -Wextra)

# std::filesystem needs an extra library on older GCC releases
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(banking_core PUBLIC stdc++fs)
endif()

# Application
add_executable(banking_system src/main.cpp)
target_link_libraries(banking_system PRIVATE banking_core)
target_compile_options(banking_system PRIVATE -Wall -Wextra)

# Tests
enable_testing()
add_executable(banking_tests tests/test_main.cpp)
target_link_libraries(banking_tests PRIVATE banking_core)
add_test(NAME banking_tests COMMAND banking_tests)
