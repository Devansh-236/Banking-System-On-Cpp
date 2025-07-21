#include "headers/Utilities.h"
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

// Get current date in YYYY-MM-DD format
string getCurrentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    
    stringstream ss;
    ss << (1900 + ltm->tm_year) << "-"
       << setfill('0') << setw(2) << (1 + ltm->tm_mon) << "-"
       << setfill('0') << setw(2) << ltm->tm_mday;
    
    return ss.str();
}

// Simple hash function for PIN security
string simpleHash(const string& input) {
    if (input.empty()) return "0";
    
    unsigned long hash = 5381;
    for (char c : input) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return to_string(hash);
}

// Validate email format (basic validation without regex)
bool isValidEmail(const string& email) {
    if (email.empty()) return false;
    
    // Check for @ symbol and basic structure
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0 || atPos == email.length() - 1) {
        return false;
    }
    
    // Check for dot after @
    size_t dotPos = email.find('.', atPos);
    if (dotPos == string::npos || dotPos == email.length() - 1) {
        return false;
    }
    
    // Check for valid characters
    for (size_t i = 0; i < email.length(); i++) {
        char c = email[i];
        if (i == atPos || i == dotPos) continue;
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }
    
    return true;
}

// Validate phone number format (basic validation without regex)
bool isValidPhone(const string& phone) {
    if (phone.empty()) return false;
    
    // Remove common separators for validation
    string cleanPhone = phone;
    cleanPhone.erase(remove(cleanPhone.begin(), cleanPhone.end(), '-'), cleanPhone.end());
    cleanPhone.erase(remove(cleanPhone.begin(), cleanPhone.end(), ' '), cleanPhone.end());
    cleanPhone.erase(remove(cleanPhone.begin(), cleanPhone.end(), '('), cleanPhone.end());
    cleanPhone.erase(remove(cleanPhone.begin(), cleanPhone.end(), ')'), cleanPhone.end());
    cleanPhone.erase(remove(cleanPhone.begin(), cleanPhone.end(), '+'), cleanPhone.end());
    
    // Check if remaining characters are digits and reasonable length
    if (cleanPhone.length() < 10 || cleanPhone.length() > 15) {
        return false;
    }
    
    for (char c : cleanPhone) {
        if (!isdigit(c)) {
            return false;
        }
    }
    
    return true;
}

// Validate customer name (letters and spaces only)
bool isValidName(const string& name) {
    if (name.empty()) return false;
    
    for (char c : name) {
        if (!isalpha(c) && c != ' ') {
            return false;
        }
    }
    
    return true;
}

// Validate account number format
bool isValidAccountNumber(const string& accountNumber) {
    if (accountNumber.empty() || accountNumber.length() < 3) {
        return false;
    }
    
    // Simple validation - starts with letters followed by numbers
    bool hasLetter = false;
    bool hasNumber = false;
    
    for (char c : accountNumber) {
        if (isalpha(c)) hasLetter = true;
        if (isdigit(c)) hasNumber = true;
    }
    
    return hasLetter && hasNumber;
}

// Validate monetary amount
bool isValidAmount(const double& amount) {
    return amount >= 0.0; // Valid amounts are non-negative
}
