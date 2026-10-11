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

// Get current month in YYYY-MM format
string getCurrentMonth();

// Validate if a date string is a real calendar date in YYYY-MM-DD format
bool isValidDate(string date);

// Validate if a month string is in YYYY-MM format
bool isValidMonth(const string& month);

// Add (or subtract, for negative values) days to a YYYY-MM-DD date
string addDaysToDate(const string& date, int days);

// =============================================================================
// VALIDATION UTILITIES
// =============================================================================

// Validate email format
bool isValidEmail(const string& email);

// Validate phone number format
bool isValidPhone(const string& phone);

// Validate customer name (letters, spaces, dots, hyphens and apostrophes)
bool isValidName(const string& name);

// Validate account number format
bool isValidAccountNumber(const string& accountNumber);

// Validate monetary amount
bool isValidAmount(const double& amount);

// Validate PIN (4 to 6 digits)
bool isValidPin(const string& pin);

// =============================================================================
// MONEY UTILITIES
// =============================================================================

// Round a monetary value to whole cents
double roundMoney(double amount);

// Format a monetary value as $1,234.56 (negative values as -$1,234.56)
string formatCurrency(double amount);

// =============================================================================
// STRING UTILITIES
// =============================================================================

// Remove leading and trailing whitespace
string trim(const string& str);

// Convert to upper case
string toUpper(string str);

// Join fields into a single '|' separated record, escaping special characters
string joinRecord(const vector<string>& fields);

// Split a record produced by joinRecord back into its fields
vector<string> splitRecord(const string& line);

// Parse helpers that return a fallback instead of throwing
double parseDouble(const string& str, double fallback = 0.0);
int parseInt(const string& str, int fallback = 0);
bool parseBool(const string& str, bool fallback = false);

// =============================================================================
// SECURITY UTILITIES
// =============================================================================

// SHA-256 digest of input, as lowercase hex
string sha256(const string& input);

// Salted hash for customer PINs
string hashPin(const string& pin, const string& salt);

// Legacy hash function (kept for compatibility, not used for new credentials)
string simpleHash(const string& input);

#endif // UTILITIES_H
