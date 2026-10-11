#include "Customer.h"
#include "Account.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

using namespace std;

// =============================================================================
// CONSTRUCTOR
// =============================================================================

// Constructor
Customer::Customer(string id, string customerName, string addr, string phone, string mail, string pin) {
    if (id.empty() || trim(customerName).empty() || trim(addr).empty() || phone.empty() || mail.empty() || pin.empty()) {
        throw invalid_argument("All fields must be provided and cannot be empty.");
    }
    if (!isValidEmail(mail)) {
        throw invalid_argument("Invalid email format.");
    }
    customerId = id;
    name = trim(customerName);
    address = trim(addr);
    phoneNumber = trim(phone);
    email = trim(mail);
    hashedPin = hashPin(pin, customerId);
    registrationDate = getCurrentDate();
    isActive = true;
    failedLoginAttempts = 0;
    isLocked = false;
}

// =============================================================================
// GETTER FUNCTIONS
// =============================================================================

// Get customer ID
string Customer::getCustomerId() const {
    return customerId;
}

// Get customer name
string Customer::getName() const {
    return name;
}

// Get customer address
string Customer::getAddress() const {
    return address;
}

// Get phone number
string Customer::getPhoneNumber() const {
    return phoneNumber;
}

// Get email address
string Customer::getEmail() const {
    return email;
}

// Get all accounts
vector<Account*> Customer::getAllAccounts() const {
    return accounts;
}

// Get registration date
string Customer::getRegistrationDate() const {
    return registrationDate;
}

// Check if customer is active
bool Customer::isCustomerActive() const {
    return isActive;
}

// Get total number of accounts
int Customer::getAccountCount() const {
    return static_cast<int>(accounts.size());
}

// =============================================================================
// ACCOUNT MANAGEMENT FUNCTIONS
// =============================================================================

// Link an account to this customer
bool Customer::addAccount(Account* account) {
    if (account == nullptr) {
        return false;
    }
    if (hasAccount(account->getAccountNumber())) {
        return false;
    }
    if (account->getCustomerId() != customerId) {
        return false;
    }
    accounts.push_back(account);
    return true;
}

// Unlink an account by account number (the Bank owns and frees the object)
bool Customer::removeAccount(string accountNumber) {
    auto it = find_if(accounts.begin(), accounts.end(),
                      [&accountNumber](Account* account) {
                          return account->getAccountNumber() == accountNumber;
                      });
    if (it == accounts.end()) {
        return false;
    }
    accounts.erase(it);
    return true;
}

// Find an account by account number
Account* Customer::findAccount(string accountNumber) const {
    for (Account* account : accounts) {
        if (account->getAccountNumber() == accountNumber) {
            return account; // Account found
        }
    }
    return nullptr; // Account not found
}

// Check if customer has a specific account
bool Customer::hasAccount(string accountNumber) const {
    return findAccount(accountNumber) != nullptr;
}

// Get accounts by type (SAVINGS or CHECKING)
vector<Account*> Customer::getAccountsByType(AccountType type) const {
    vector<Account*> filteredAccounts;
    for (Account* account : accounts) {
        if (account->getAccountType() == type) {
            filteredAccounts.push_back(account);
        }
    }
    return filteredAccounts;
}

// =============================================================================
// AUTHENTICATION FUNCTIONS
// =============================================================================

// Verify customer PIN
bool Customer::verifyPin(string inputPin) const {
    if (inputPin.empty()) {
        return false;
    }
    return hashPin(inputPin, customerId) == hashedPin;
}

// Change customer PIN
bool Customer::changePin(string oldPin, string newPin) {
    if (!verifyPin(oldPin)) {
        return false;
    }
    return resetPin(newPin);
}

// Set a new PIN without the old one (admin reset)
bool Customer::resetPin(string newPin) {
    if (!isValidPin(newPin)) {
        return false;
    }
    hashedPin = hashPin(newPin, customerId);
    return true;
}

// =============================================================================
// LOGIN LOCKOUT
// =============================================================================

int Customer::getFailedLoginAttempts() const {
    return failedLoginAttempts;
}

bool Customer::isAccountLocked() const {
    return isLocked;
}

// Record a failed login; lock the customer once maxAttempts is reached
bool Customer::recordFailedLogin(int maxAttempts) {
    failedLoginAttempts++;
    if (maxAttempts > 0 && failedLoginAttempts >= maxAttempts) {
        isLocked = true;
    }
    return isLocked;
}

void Customer::resetFailedLogins() {
    failedLoginAttempts = 0;
}

void Customer::setLocked(bool locked) {
    isLocked = locked;
    if (!locked) {
        failedLoginAttempts = 0;
    }
}

// =============================================================================
// INFORMATION UPDATE FUNCTIONS
// =============================================================================

// Update contact information; empty values keep the current value
bool Customer::updateContactInfo(string newAddress, string newPhone, string newEmail) {
    newAddress = trim(newAddress);
    newPhone = trim(newPhone);
    newEmail = trim(newEmail);
    if (!newEmail.empty() && !isValidEmail(newEmail)) {
        return false;
    }
    if (!newPhone.empty() && !isValidPhone(newPhone)) {
        return false;
    }
    if (!newAddress.empty()) address = newAddress;
    if (!newPhone.empty()) phoneNumber = newPhone;
    if (!newEmail.empty()) email = newEmail;
    return true;
}

// Set customer active status (deactivating also freezes every account)
void Customer::setActive(bool status) {
    isActive = status;
    if (!isActive) {
        for (Account* account : accounts) {
            account->setActive(false);
        }
    }
}

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================

// Display basic customer information
void Customer::displayCustomerInfo() const {
    cout << "Customer ID       : " << customerId << endl;
    cout << "Name              : " << name << endl;
    cout << "Address           : " << address << endl;
    cout << "Phone             : " << (phoneNumber.empty() ? "Not provided" : phoneNumber) << endl;
    cout << "Email             : " << (email.empty() ? "Not provided" : email) << endl;
    cout << "Registration Date : " << registrationDate << endl;
    cout << "Status            : " << (isActive ? "Active" : "Inactive")
         << (isLocked ? " (LOCKED)" : "") << endl;
}

// Display account summary (brief info about all accounts)
void Customer::displayAccountSummary() const {
    cout << "Account Summary for " << name << " (" << customerId << ")" << endl;
    cout << string(62, '-') << endl;
    cout << left << setw(16) << "Account Number"
         << setw(12) << "Type"
         << right << setw(20) << "Balance"
         << "   " << left << setw(10) << "Status" << endl;
    cout << string(62, '-') << endl;
    for (const Account* account : accounts) {
        cout << left << setw(16) << account->getAccountNumber()
             << setw(12) << account->getAccountTypeString()
             << right << setw(20) << formatCurrency(account->getBalance())
             << "   " << left << setw(10) << (account->isAccountActive() ? "Active" : "Frozen") << endl;
    }
    if (accounts.empty()) {
        cout << "No accounts yet." << endl;
    }
    cout << string(62, '-') << endl;
    cout << "Savings Total  : " << formatCurrency(getTotalSavingsBalance()) << endl;
    cout << "Checking Total : " << formatCurrency(getTotalCheckingBalance()) << endl;
    cout << "Overall Total  : " << formatCurrency(getTotalBalance())
         << " across " << getTotalActiveAccounts() << " active account(s)" << endl;
}

// Display detailed information for all accounts
void Customer::displayDetailedAccountInfo() const {
    cout << "Detailed Account Information for " << name << " (" << customerId << ")" << endl;
    cout << string(50, '-') << endl;
    for (const Account* account : accounts) {
        account->displayAccountInfo();
        cout << string(50, '-') << endl;
    }
    if (accounts.empty()) {
        cout << "No accounts found for this customer." << endl;
    }
}

// =============================================================================
// FINANCIAL SUMMARY FUNCTIONS
// =============================================================================

// Get total balance across all accounts
double Customer::getTotalBalance() const {
    double totalBalance = 0.0;
    for (const Account* account : accounts) {
        totalBalance += account->getBalance();
    }
    return roundMoney(totalBalance);
}

// Get total savings account balance
double Customer::getTotalSavingsBalance() const {
    double totalSavingsBalance = 0.0;
    for (const Account* account : getAccountsByType(AccountType::SAVINGS)) {
        totalSavingsBalance += account->getBalance();
    }
    return roundMoney(totalSavingsBalance);
}

// Get total checking account balance
double Customer::getTotalCheckingBalance() const {
    double totalCheckingBalance = 0.0;
    for (const Account* account : getAccountsByType(AccountType::CHECKING)) {
        totalCheckingBalance += account->getBalance();
    }
    return roundMoney(totalCheckingBalance);
}

// Get count of active accounts
int Customer::getTotalActiveAccounts() const {
    int activeCount = 0;
    for (const Account* account : accounts) {
        if (account->isAccountActive()) {
            activeCount++;
        }
    }
    return activeCount;
}
