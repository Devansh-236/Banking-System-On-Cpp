#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>
#include<climits>

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
// ID GENERATION UTILITIES
// =============================================================================

// Generate unique transaction ID
string generateTransactionId();

// Generate random ID of specified length
string generateRandomId(int length);

// Generate next account number based on type
string generateAccountNumber(string accountType);

// Generate next customer ID
string generateCustomerId();

// Generate next loan ID
string generateLoanId();

// =============================================================================
// VALIDATION UTILITIES
// =============================================================================

// Validate account number format
bool isValidAccountNumber(string accountNumber);

// Validate monetary amount
bool isValidAmount(double amount);

// Validate PIN format (4 digits)
bool isValidPin(string pin);

// Validate email format
bool isValidEmail(string email);

// Validate phone number format
bool isValidPhone(string phone);

// Validate customer name (letters and spaces only)
bool isValidName(string name);

// =============================================================================
// STRING UTILITIES
// =============================================================================

// Remove leading and trailing whitespace
string trim(string str);

// Convert string to uppercase
string toUpperCase(string str);

// Convert string to lowercase
string toLowerCase(string str);

// Split string by delimiter
vector<string> split(string str, char delimiter);

// Check if string contains only digits
bool isNumeric(string str);

// Check if string contains only alphabetic characters
bool isAlphabetic(string str);

// =============================================================================
// FORMATTING UTILITIES
// =============================================================================

// Format currency amount (e.g., "$1,234.56")
string formatCurrency(double amount);

// Format date for display (e.g., "January 22, 2025")
string formatDateForDisplay(string date);

// Format account number for display (e.g., "****1234")
string maskAccountNumber(string accountNumber);

// Center text within specified width
string centerText(string text, int width);

// =============================================================================
// SECURITY UTILITIES
// =============================================================================

// Simple hash function for passwords/PINs
string simpleHash(string input);

// Encrypt data with simple Caesar cipher
string simpleEncrypt(string data, int shift = 3);

// Decrypt data with simple Caesar cipher
string simpleDecrypt(string encryptedData, int shift = 3);

// Generate random salt for hashing
string generateSalt();

// =============================================================================
// MATHEMATICAL UTILITIES
// =============================================================================

// Calculate compound interest
double calculateCompoundInterest(double principal, double rate, int timeMonths, int compoundFreq = 12);

// Calculate simple interest
double calculateSimpleInterest(double principal, double rate, int timeMonths);

// Calculate monthly loan payment using PMT formula
double calculateLoanPayment(double principal, double annualRate, int months);

// Round to 2 decimal places for currency
double roundToCurrency(double amount);

// =============================================================================
// FILE UTILITIES
// =============================================================================

// Check if file exists
bool fileExists(string filename);

// Get file size in bytes
long getFileSize(string filename);

// Create directory if it doesn't exist
bool createDirectory(string path);

// Get current working directory
string getCurrentDirectory();

// =============================================================================
// INPUT/OUTPUT UTILITIES
// =============================================================================

// Clear console screen (cross-platform)
void clearScreen();

// Pause and wait for user input
void pauseScreen();

// Display a horizontal line separator
void printSeparator(int length = 50);

// Display formatted header
void printHeader(string title);

// Get integer input with validation
int getValidInteger(string prompt, int min = INT_MIN, int max = INT_MAX);

// Get double input with validation
double getValidDouble(string prompt, double min = -999999.99, double max = 999999.99);

// Get string input with validation
string getValidString(string prompt, int minLength = 1, int maxLength = 100);

// Get password input (masked)
string getPasswordInput(string prompt);

// Display success message in green (if terminal supports colors)
void showSuccessMessage(string message);

// Display error message in red (if terminal supports colors)
void showErrorMessage(string message);

// Display warning message in yellow (if terminal supports colors)
void showWarningMessage(string message);

// =============================================================================
// SYSTEM UTILITIES
// =============================================================================

// Get system timestamp as string
string getSystemTimestamp();

// Sleep for specified milliseconds
void sleepMs(int milliseconds);

// Generate unique session ID
string generateSessionId();

// Log message to file
void logToFile(string message, string logLevel = "INFO");

#endif // UTILITIES_H
