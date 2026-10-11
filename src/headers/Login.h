#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include <ctime>
#include "Bank.h"

using namespace std;

// Outcome of a login attempt
enum class LoginResult {
    SUCCESS,
    INVALID_CREDENTIALS,
    ACCOUNT_LOCKED,
    INACTIVE
};

// Handles customer and administrator authentication, login lockout and
// session lifetime. Admin credentials and security policy are read from
// config/admin_settings.txt (password stored as a SHA-256 hash).
class LoginManager {
private:
    Bank& bank;
    string adminConfigPath;

    // Admin settings
    string adminUsername;
    string adminPasswordHash;
    int passwordMinLength;
    bool requireSpecialChars;
    int adminSessionTimeout;
    bool autoBackup;
    int logRetentionDays;
    unsigned long long maxLogSize;
    int adminFailedAttempts;

    // Current session
    bool loggedIn;
    bool adminSession;
    string currentUserId;
    string sessionId;
    time_t loginTime;
    time_t lastActivity;

    string lastMessage;

    string generateSessionId() const;
    void startSession(const string& userId, bool isAdmin);

public:
    // Constructor
    LoginManager(Bank& bankRef, string adminConfig = "config/admin_settings.txt");

    // Settings
    bool loadAdminSettings();
    bool isAutoBackupEnabled() const;
    int getLogRetentionDays() const;
    unsigned long long getMaxLogSize() const;

    // Authentication
    LoginResult loginCustomer(string customerId, string pin);
    LoginResult loginAdmin(string username, string password);
    void logout();

    // Session state
    bool isLoggedIn() const;
    bool isAdmin() const;
    string getCurrentUserId() const;
    string getSessionId() const;
    int getSessionTimeout() const;      // Seconds of inactivity allowed for the current session
    bool touchSession();                // Refreshes activity; returns false (and logs out) if expired
    string getLoginTimeString() const;

    // Admin password management
    bool validatePasswordStrength(const string& password, string& error) const;
    bool changeAdminPassword(string currentPassword, string newPassword);

    string getLastMessage() const;
};

#endif // LOGIN_H
