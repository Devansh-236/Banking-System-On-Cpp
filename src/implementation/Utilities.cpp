#include "Utilities.h"
#include <ctime>
#include <cmath>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

// =============================================================================
// DATE AND TIME UTILITIES
// =============================================================================

// Format the current local time with the given strftime pattern
static string formatNow(const char* pattern) {
    time_t now = time(nullptr);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32];
    strftime(buffer, sizeof(buffer), pattern, &local);
    return string(buffer);
}

// Get current date in YYYY-MM-DD format
string getCurrentDate() {
    return formatNow("%Y-%m-%d");
}

// Get current time in HH:MM:SS format
string getCurrentTime() {
    return formatNow("%H:%M:%S");
}

// Get current date and time in YYYY-MM-DD HH:MM:SS format
string getCurrentDateTime() {
    return formatNow("%Y-%m-%d %H:%M:%S");
}

// Get current month in YYYY-MM format
string getCurrentMonth() {
    return formatNow("%Y-%m");
}

// Parse YYYY-MM-DD into a tm structure (noon, to stay clear of DST edges)
static bool parseDate(const string& date, tm& out) {
    if (date.length() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (size_t i = 0; i < date.length(); i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    out = tm{};
    out.tm_year = stoi(date.substr(0, 4)) - 1900;
    out.tm_mon = stoi(date.substr(5, 2)) - 1;
    out.tm_mday = stoi(date.substr(8, 2));
    out.tm_hour = 12;
    out.tm_isdst = -1;
    return true;
}

// Validate if a date string is a real calendar date in YYYY-MM-DD format
bool isValidDate(string date) {
    tm parsed{};
    if (!parseDate(date, parsed)) return false;
    int year = parsed.tm_year, month = parsed.tm_mon, day = parsed.tm_mday;
    if (month < 0 || month > 11 || day < 1 || day > 31) return false;
    // mktime normalises out-of-range days (e.g. Feb 30 -> Mar 2), so compare back
    tm normalised = parsed;
    if (mktime(&normalised) == -1) return false;
    return normalised.tm_year == year && normalised.tm_mon == month && normalised.tm_mday == day;
}

// Validate if a month string is in YYYY-MM format
bool isValidMonth(const string& month) {
    return isValidDate(month + "-01");
}

// Add (or subtract, for negative values) days to a YYYY-MM-DD date
string addDaysToDate(const string& date, int days) {
    tm parsed{};
    if (!parseDate(date, parsed)) return date;
    parsed.tm_mday += days;
    mktime(&parsed);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &parsed);
    return string(buffer);
}

// =============================================================================
// VALIDATION UTILITIES
// =============================================================================

// Validate email format (basic validation without regex)
bool isValidEmail(const string& email) {
    if (email.empty()) return false;

    // Exactly one @, not at either end
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0 || atPos == email.length() - 1) {
        return false;
    }
    if (email.find('@', atPos + 1) != string::npos) {
        return false;
    }

    // Domain needs a dot that is neither right after @ nor at the end
    size_t dotPos = email.find('.', atPos);
    if (dotPos == string::npos || dotPos == atPos + 1 || email.back() == '.') {
        return false;
    }

    // Check for valid characters
    for (size_t i = 0; i < email.length(); i++) {
        char c = email[i];
        if (i == atPos) continue;
        if (!isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-' && c != '+') {
            return false;
        }
    }

    return true;
}

// Validate phone number format (basic validation without regex)
bool isValidPhone(const string& phone) {
    if (phone.empty()) return false;

    // Remove common separators for validation
    string cleanPhone;
    for (char c : phone) {
        if (c == '-' || c == ' ' || c == '(' || c == ')') continue;
        if (c == '+' && cleanPhone.empty()) continue;
        cleanPhone += c;
    }

    // Check if remaining characters are digits and reasonable length
    if (cleanPhone.length() < 7 || cleanPhone.length() > 15) {
        return false;
    }

    for (char c : cleanPhone) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    return true;
}

// Validate customer name (letters, spaces, dots, hyphens and apostrophes)
bool isValidName(const string& name) {
    string trimmed = trim(name);
    if (trimmed.empty() || trimmed.length() > 60) return false;

    bool hasLetter = false;
    for (char c : trimmed) {
        if (isalpha(static_cast<unsigned char>(c))) {
            hasLetter = true;
        } else if (c != ' ' && c != '.' && c != '-' && c != '\'') {
            return false;
        }
    }

    return hasLetter;
}

// Validate account number format: SAV or CHK followed by 8 digits
bool isValidAccountNumber(const string& accountNumber) {
    if (accountNumber.length() != 11) {
        return false;
    }
    string prefix = accountNumber.substr(0, 3);
    if (prefix != "SAV" && prefix != "CHK") {
        return false;
    }
    for (size_t i = 3; i < accountNumber.length(); i++) {
        if (!isdigit(static_cast<unsigned char>(accountNumber[i]))) return false;
    }
    return true;
}

// Validate monetary amount: positive, finite, and not absurdly large
bool isValidAmount(const double& amount) {
    return std::isfinite(amount) && amount > 0.0 && amount <= 1e12;
}

// Validate PIN (4 to 6 digits)
bool isValidPin(const string& pin) {
    if (pin.length() < 4 || pin.length() > 6) return false;
    return all_of(pin.begin(), pin.end(), [](char c) { return isdigit(static_cast<unsigned char>(c)); });
}

// =============================================================================
// MONEY UTILITIES
// =============================================================================

// Round a monetary value to whole cents
double roundMoney(double amount) {
    double rounded = std::round(amount * 100.0) / 100.0;
    return rounded == 0.0 ? 0.0 : rounded; // avoid printing -0.00
}

// Format a monetary value as $1,234.56 (negative values as -$1,234.56)
string formatCurrency(double amount) {
    amount = roundMoney(amount);
    bool negative = amount < 0;
    stringstream ss;
    ss << fixed << setprecision(2) << (negative ? -amount : amount);
    string digits = ss.str();

    size_t dot = digits.find('.');
    string whole = digits.substr(0, dot);
    string fraction = digits.substr(dot);
    string grouped;
    int counter = 0;
    for (auto it = whole.rbegin(); it != whole.rend(); ++it) {
        if (counter > 0 && counter % 3 == 0) grouped += ',';
        grouped += *it;
        counter++;
    }
    reverse(grouped.begin(), grouped.end());
    return (negative ? "-$" : "$") + grouped + fraction;
}

// =============================================================================
// STRING UTILITIES
// =============================================================================

// Remove leading and trailing whitespace
string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Convert to upper case
string toUpper(string str) {
    transform(str.begin(), str.end(), str.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    return str;
}

// Join fields into a single '|' separated record, escaping special characters
string joinRecord(const vector<string>& fields) {
    string line;
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0) line += '|';
        for (char c : fields[i]) {
            switch (c) {
            case '\\': line += "\\\\"; break;
            case '|':  line += "\\p"; break;
            case '\n': line += "\\n"; break;
            case '\r': break;
            default:   line += c;
            }
        }
    }
    return line;
}

// Split a record produced by joinRecord back into its fields
vector<string> splitRecord(const string& line) {
    vector<string> fields;
    string current;
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.length()) {
            char next = line[++i];
            if (next == 'p') current += '|';
            else if (next == 'n') current += '\n';
            else current += next;
        } else if (c == '|') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

double parseDouble(const string& str, double fallback) {
    try {
        size_t used = 0;
        double value = stod(trim(str), &used);
        return std::isfinite(value) ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

int parseInt(const string& str, int fallback) {
    try {
        return stoi(trim(str));
    } catch (...) {
        return fallback;
    }
}

bool parseBool(const string& str, bool fallback) {
    string value = toUpper(trim(str));
    if (value == "TRUE" || value == "1" || value == "YES") return true;
    if (value == "FALSE" || value == "0" || value == "NO") return false;
    return fallback;
}

// =============================================================================
// SECURITY UTILITIES
// =============================================================================

// SHA-256 (FIPS 180-4), self-contained so the project has no external dependencies
string sha256(const string& input) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };

    // Pad message: 0x80, zeros, then 64-bit big-endian bit length
    string message = input;
    uint64_t bitLength = static_cast<uint64_t>(input.size()) * 8;
    message += static_cast<char>(0x80);
    while (message.size() % 64 != 56) message += static_cast<char>(0x00);
    for (int i = 7; i >= 0; i--) message += static_cast<char>((bitLength >> (i * 8)) & 0xff);

    for (size_t chunk = 0; chunk < message.size(); chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = (static_cast<uint32_t>(static_cast<unsigned char>(message[chunk + i * 4])) << 24) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(message[chunk + i * 4 + 1])) << 16) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(message[chunk + i * 4 + 2])) << 8) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(message[chunk + i * 4 + 3])));
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = hh + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    stringstream ss;
    for (uint32_t value : h) {
        ss << hex << setw(8) << setfill('0') << value;
    }
    return ss.str();
}

// Salted hash for customer PINs (salt is the customer ID, so equal PINs hash differently)
string hashPin(const string& pin, const string& salt) {
    return sha256(salt + ":" + pin);
}

// Legacy hash function (djb2)
string simpleHash(const string& input) {
    if (input.empty()) return "0";

    unsigned long hash = 5381;
    for (char c : input) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash * 33 + c
    }
    return to_string(hash);
}
