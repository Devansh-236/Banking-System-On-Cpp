#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>
#include <climits>

using namespace std;

// =============================================================================
// DATE AND TIME UTILITIES
// =============================================================================

// Get current date in YYYY-MM-DD format
string getCurrentDate();

// Get current time in HH:MM:SS format
string getCurrentTime();

// Get current date and time in YYYY-MM-DD HH:MM:SS format
string getCurrentDateTime();

// Validate if a date string is in correct format
bool isValidDate(string date);

// =============================================================================
// VALIDATION UTILITIES
// =============================================================================

// Validate email format
bool isValidEmail(const string& email);

// Validate phone number format
bool isValidPhone(const string& phone);

// Validate customer name (letters and spaces only)
bool isValidName(const string& name);

// Validate account number format
bool isValidAccountNumber(const string& accountNumber);

// Validate monetary amount
bool isValidAmount(const double& amount);

// =============================================================================
// SECURITY UTILITIES
// =============================================================================

// Simple hash function for passwords/PINs
string simpleHash(const string& input);

#endif // UTILITIES_H
// =============================================================================