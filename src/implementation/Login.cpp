#include "Login.h"
#include "FileManager.h"
#include "Utilities.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>

using namespace std;

// SHA-256 of "password", the documented default admin password
static const string DEFAULT_ADMIN_HASH = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";

// =============================================================================
// CONSTRUCTOR AND SETTINGS
// =============================================================================

LoginManager::LoginManager(Bank& bankRef, string adminConfig)
    : bank(bankRef) {
    adminConfigPath = adminConfig;
    adminUsername = "admin";
    adminPasswordHash = DEFAULT_ADMIN_HASH;
    passwordMinLength = 8;
    requireSpecialChars = true;
    adminSessionTimeout = 3600;
    autoBackup = true;
    logRetentionDays = 30;
    maxLogSize = 10ULL * 1024 * 1024;
    adminFailedAttempts = 0;
    loggedIn = false;
    adminSession = false;
    loginTime = 0;
    lastActivity = 0;
}

bool LoginManager::loadAdminSettings() {
    map<string, string> settings;
    if (!FileManager::loadKeyValueFile(adminConfigPath, settings)) {
        lastMessage = "Admin settings file " + adminConfigPath + " not found; using defaults.";
        return false;
    }
    if (!settings["ADMIN_USERNAME"].empty()) adminUsername = settings["ADMIN_USERNAME"];
    // A settings file without a valid hash disables admin login rather than
    // silently falling back to the default password
    string hash = toUpper(trim(settings["ADMIN_PASSWORD_HASH"]));
    bool validHash = hash.size() == 64 &&
                     all_of(hash.begin(), hash.end(), [](char c) { return isxdigit(static_cast<unsigned char>(c)); });
    adminPasswordHash = validHash ? hash : "";
    passwordMinLength = max(4, parseInt(settings["PASSWORD_MIN_LENGTH"], passwordMinLength));
    requireSpecialChars = parseBool(settings["REQUIRE_SPECIAL_CHARS"], requireSpecialChars);
    adminSessionTimeout = max(0, parseInt(settings["SESSION_TIMEOUT"], adminSessionTimeout));
    autoBackup = parseBool(settings["AUTO_BACKUP"], autoBackup);
    logRetentionDays = max(1, parseInt(settings["LOG_RETENTION_DAYS"], logRetentionDays));
    maxLogSize = FileManager::parseSize(settings["MAX_LOG_SIZE"], maxLogSize);
    return true;
}

bool LoginManager::isAutoBackupEnabled() const {
    return autoBackup;
}

int LoginManager::getLogRetentionDays() const {
    return logRetentionDays;
}

unsigned long long LoginManager::getMaxLogSize() const {
    return maxLogSize;
}

// =============================================================================
// AUTHENTICATION
// =============================================================================

string LoginManager::generateSessionId() const {
    static mt19937_64 generator(random_device{}());
    stringstream ss;
    ss << "S" << hex << uppercase << setw(12) << setfill('0') << (generator() & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

void LoginManager::startSession(const string& userId, bool isAdmin) {
    loggedIn = true;
    adminSession = isAdmin;
    currentUserId = userId;
    sessionId = generateSessionId();
    loginTime = time(nullptr);
    lastActivity = loginTime;
    bank.setSessionContext(sessionId);
}

LoginResult LoginManager::loginCustomer(string customerId, string pin) {
    logout();
    customerId = toUpper(trim(customerId));
    Customer* customer = bank.findCustomer(customerId);
    if (!customer) {
        lastMessage = "Invalid customer ID or PIN.";
        bank.getFileManager().logSystem("Failed login for unknown customer ID " + customerId);
        return LoginResult::INVALID_CREDENTIALS;
    }
    if (customer->isAccountLocked()) {
        lastMessage = "This profile is locked after too many failed attempts. Please contact the bank.";
        return LoginResult::ACCOUNT_LOCKED;
    }
    if (!customer->isCustomerActive()) {
        lastMessage = "This profile is inactive. Please contact the bank.";
        return LoginResult::INACTIVE;
    }
    if (!customer->verifyPin(pin)) {
        int maxAttempts = bank.getMaxLoginAttempts();
        bool locked = customer->recordFailedLogin(maxAttempts);
        bank.saveAllData();
        bank.getFileManager().logSystem("Failed login for " + customerId);
        if (locked) {
            bank.getFileManager().logSystem("Customer " + customerId + " locked after " +
                                            to_string(maxAttempts) + " failed attempts");
            lastMessage = "Too many failed attempts. This profile is now locked; please contact the bank.";
            return LoginResult::ACCOUNT_LOCKED;
        }
        int remaining = maxAttempts - customer->getFailedLoginAttempts();
        lastMessage = "Invalid customer ID or PIN. " + to_string(remaining) + " attempt(s) remaining.";
        return LoginResult::INVALID_CREDENTIALS;
    }
    if (customer->getFailedLoginAttempts() > 0) {
        customer->resetFailedLogins();
        bank.saveAllData();
    }
    startSession(customerId, false);
    bank.getFileManager().logSystem("Customer " + customerId + " logged in (session " + sessionId + ")");
    lastMessage = "Welcome, " + customer->getName() + "!";
    return LoginResult::SUCCESS;
}

LoginResult LoginManager::loginAdmin(string username, string password) {
    logout();
    int maxAttempts = bank.getMaxLoginAttempts();
    if (adminPasswordHash.empty()) {
        lastMessage = "Admin login is disabled: ADMIN_PASSWORD_HASH in " + adminConfigPath + " is missing or invalid.";
        return LoginResult::INVALID_CREDENTIALS;
    }
    if (adminFailedAttempts >= maxAttempts) {
        lastMessage = "Admin login is locked for this run after too many failed attempts. Restart the application.";
        return LoginResult::ACCOUNT_LOCKED;
    }
    if (trim(username) != adminUsername || toUpper(sha256(password)) != toUpper(adminPasswordHash)) {
        adminFailedAttempts++;
        bank.getFileManager().logSystem("Failed admin login for username '" + trim(username) + "'");
        if (adminFailedAttempts >= maxAttempts) {
            lastMessage = "Too many failed attempts. Admin login is locked until restart.";
            return LoginResult::ACCOUNT_LOCKED;
        }
        lastMessage = "Invalid admin credentials. " + to_string(maxAttempts - adminFailedAttempts) +
                      " attempt(s) remaining.";
        return LoginResult::INVALID_CREDENTIALS;
    }
    adminFailedAttempts = 0;
    startSession(adminUsername, true);
    bank.getFileManager().logSystem("Admin logged in (session " + sessionId + ")");
    lastMessage = "Admin access granted.";
    if (toUpper(adminPasswordHash) == toUpper(DEFAULT_ADMIN_HASH)) {
        lastMessage += " You are using the default password - please change it.";
    }
    return LoginResult::SUCCESS;
}

void LoginManager::logout() {
    if (loggedIn) {
        bank.getFileManager().logSystem((adminSession ? string("Admin") : "Customer " + currentUserId) +
                                        " logged out (session " + sessionId + ")");
    }
    loggedIn = false;
    adminSession = false;
    currentUserId.clear();
    sessionId.clear();
    bank.setSessionContext("");
}

// =============================================================================
// SESSION STATE
// =============================================================================

bool LoginManager::isLoggedIn() const {
    return loggedIn;
}

bool LoginManager::isAdmin() const {
    return loggedIn && adminSession;
}

string LoginManager::getCurrentUserId() const {
    return currentUserId;
}

string LoginManager::getSessionId() const {
    return sessionId;
}

int LoginManager::getSessionTimeout() const {
    return adminSession ? adminSessionTimeout : bank.getSessionTimeout();
}

bool LoginManager::touchSession() {
    if (!loggedIn) return false;
    time_t now = time(nullptr);
    int timeout = getSessionTimeout();
    if (timeout > 0 && difftime(now, lastActivity) > timeout) {
        bank.getFileManager().logSystem("Session " + sessionId + " expired");
        logout();
        lastMessage = "Your session expired after " +
                      (timeout >= 60 ? to_string(timeout / 60) + " minute(s)" : to_string(timeout) + " second(s)") +
                      " of inactivity. Please log in again.";
        return false;
    }
    lastActivity = now;
    return true;
}

string LoginManager::getLoginTimeString() const {
    if (!loggedIn) return "";
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &loginTime);
#else
    localtime_r(&loginTime, &local);
#endif
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}

// =============================================================================
// ADMIN PASSWORD MANAGEMENT
// =============================================================================

bool LoginManager::validatePasswordStrength(const string& password, string& error) const {
    if (static_cast<int>(password.length()) < passwordMinLength) {
        error = "Password must be at least " + to_string(passwordMinLength) + " characters.";
        return false;
    }
    bool hasLetter = false, hasDigit = false, hasSpecial = false;
    for (char c : password) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (isalpha(uc)) hasLetter = true;
        else if (isdigit(uc)) hasDigit = true;
        else if (!isspace(uc)) hasSpecial = true;
    }
    if (!hasLetter || !hasDigit) {
        error = "Password must contain both letters and digits.";
        return false;
    }
    if (requireSpecialChars && !hasSpecial) {
        error = "Password must contain at least one special character.";
        return false;
    }
    return true;
}

bool LoginManager::changeAdminPassword(string currentPassword, string newPassword) {
    if (!isAdmin()) {
        lastMessage = "Only a logged-in administrator can change the admin password.";
        return false;
    }
    if (toUpper(sha256(currentPassword)) != toUpper(adminPasswordHash)) {
        lastMessage = "Current password is incorrect.";
        return false;
    }
    string error;
    if (!validatePasswordStrength(newPassword, error)) {
        lastMessage = error;
        return false;
    }
    string newHash = sha256(newPassword);
    if (!FileManager::updateKeyValueFile(adminConfigPath, "ADMIN_PASSWORD_HASH", newHash)) {
        lastMessage = "Could not write " + adminConfigPath + ".";
        return false;
    }
    adminPasswordHash = newHash;
    bank.getFileManager().logSystem("Admin password changed");
    lastMessage = "Admin password changed.";
    return true;
}

string LoginManager::getLastMessage() const {
    return lastMessage;
}
