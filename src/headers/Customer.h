#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <vector>
#include "Account.h"
class Bank;
using namespace std;

// A bank customer. The Bank owns all Account objects; a Customer only keeps
// non-owning pointers to the accounts it holds.
class Customer {
    friend class Bank; // Grant Bank access to private members (persistence, PIN hash)
private:
    string customerId;
    string name;
    string address;
    string phoneNumber;
    string email;
    string hashedPin;           // Salted SHA-256 hash of the PIN
    vector<Account*> accounts;  // Non-owning pointers, owned by Bank
    string registrationDate;
    bool isActive;
    int failedLoginAttempts;
    bool isLocked;

protected:
    string getHashedPin() const {
        return hashedPin; // For internal use, not exposed publicly
    }
public:
    // Constructor (throws invalid_argument on bad input)
    Customer(string id, string customerName, string addr, string phone, string mail, string pin);

    // Destructor
    ~Customer() = default;

    // Copy constructor and assignment operator (prevent shallow copying)
    Customer(const Customer& other) = delete;
    Customer& operator=(const Customer& other) = delete;

    // Getters
    string getCustomerId() const;
    string getName() const;
    string getAddress() const;
    string getPhoneNumber() const;
    string getEmail() const;
    vector<Account*> getAllAccounts() const;
    string getRegistrationDate() const;
    bool isCustomerActive() const;
    int getAccountCount() const;

    // Account management (links only; never deletes accounts)
    bool addAccount(Account* account);
    bool removeAccount(string accountNumber);
    Account* findAccount(string accountNumber) const;
    bool hasAccount(string accountNumber) const;
    vector<Account*> getAccountsByType(AccountType type) const;

    // Authentication
    bool verifyPin(string inputPin) const;
    bool changePin(string oldPin, string newPin);
    bool resetPin(string newPin);       // Admin reset, no old PIN required

    // Login lockout tracking
    int getFailedLoginAttempts() const;
    bool isAccountLocked() const;
    bool recordFailedLogin(int maxAttempts); // Returns true if the customer is now locked
    void resetFailedLogins();
    void setLocked(bool locked);

    // Information update (empty values keep the current value)
    bool updateContactInfo(string newAddress, string newPhone, string newEmail);
    void setActive(bool status);

    // Display functions
    void displayCustomerInfo() const;
    void displayAccountSummary() const;
    void displayDetailedAccountInfo() const;

    // Financial summary
    double getTotalBalance() const;
    double getTotalSavingsBalance() const;
    double getTotalCheckingBalance() const;
    int getTotalActiveAccounts() const;
};

#endif // CUSTOMER_H
