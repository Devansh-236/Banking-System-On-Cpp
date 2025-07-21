#include "headers/Customer.h"
#include "headers/Account.h"
#include "headers/Utilities.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

// =============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// =============================================================================

// Constructor
Customer::Customer(string id, string customerName, string addr, string phone, string mail, string pin) {
    if (id.empty() || customerName.empty() || addr.empty() || phone.empty() || mail.empty() || pin.empty()) {
        throw invalid_argument("All fields must be provided and cannot be empty.");
    }
    if (!isValidEmail(mail)) {
        throw invalid_argument("Invalid email format.");
    }
    customerId = id;
    name = customerName;
    address = addr;
    phoneNumber = phone;
    email = mail;
    hashedPin = simpleHash(pin);
    registrationDate = getCurrentDate();
    isActive = true;
    accounts = vector<Account*>();
    // Display success message
    cout << "Customer created successfully with ID: " << customerId << endl;
    cout << "Registration Date: " << registrationDate << endl;
    cout << "Customer is active: " << (isActive ? "Yes" : "No") << endl;
    cout << "Customer Name: " << name << endl;
    cout << "Customer Address: " << address << endl;
    cout << "Customer Phone: " << phoneNumber << endl;
    cout << "Customer Email: " << email << endl;
    cout << "Customer PIN (hashed): " << hashedPin << endl;
    cout << "Customer accounts initialized (empty)." << endl;
    cout << "----------------------------------------" << endl;
}


// Destructor
Customer::~Customer() {
    for (Account* account : accounts) {
        delete account; // Assuming Account objects are dynamically allocated
    }
    accounts.clear();
    cout << "Customer with ID: " << customerId << " has been deleted." << endl;
    cout << "All associated accounts have been cleaned up." << endl;
    cout << "----------------------------------------" << endl;
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
    if (phoneNumber.empty()) {
        return "Phone number not provided.";
    }
    return phoneNumber;
}

// Get email address
string Customer::getEmail() const {
    if (email.empty()) {
        return "Email not provided.";
    }
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
    return accounts.size();
}

// =============================================================================
// ACCOUNT MANAGEMENT FUNCTIONS
// =============================================================================

// Add an account to customer
void Customer::addAccount(Account* account) {
    if (account == nullptr) {
        cout << "Error: Cannot add a null account." << endl;
        return;
    }
    if (hasAccount(account->getAccountNumber())) {
        cout << "Error: Account with number " << account->getAccountNumber() << " already exists for this customer." << endl;
        return;
    }
    if (account->getCustomerId() != customerId) {
        cout << "Error: Account does not belong to this customer." << endl;
        return;
    }
    accounts.push_back(account);
    cout << "Account with number " << account->getAccountNumber() << " added successfully." << endl;
    cout << "Account Type: " << (account->getAccountType() == AccountType::SAVINGS ? "SAVINGS" : "CHECKING") << endl;
    cout << "Account Balance: $" << fixed << setprecision(2) << account->getBalance() << endl;
    cout << "Account Status: " << (account->isAccountActive() ? "Active" : "Inactive") << endl;
    cout << "----------------------------------------" << endl;
}

// Remove an account by account number
bool Customer::removeAccount(string accountNumber) {
    if (accountNumber.empty()) {
        cout << "Error: Account number cannot be empty." << endl;
        return false;
    }
    if (!hasAccount(accountNumber)) {
        cout << "Error: Account with number " << accountNumber << " does not exist." << endl;
        return false;
    }

    auto it = std::remove_if(accounts.begin(), accounts.end(),
        [&accountNumber](Account* account) {
            return account->getAccountNumber() == accountNumber;
        });
    if (it != accounts.end()) {
        delete *it; // Delete the account object
        accounts.erase(it, accounts.end()); // Remove from vector
        cout << "Account with number " << accountNumber << " removed successfully." << endl;
        return true;
    } else {
        cout << "Error: Account with number " << accountNumber << " not found." << endl;
        return false;
    }
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
    Account* account = findAccount(accountNumber);
    return (account != nullptr);
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
        cout << "Error: PIN cannot be empty." << endl;
        return false;
    }
    string hashedInputPin = simpleHash(inputPin);
    if (hashedInputPin == hashedPin) {
        cout << "PIN verification successful." << endl;
        return true;
    } else {
        cout << "Error: Incorrect PIN." << endl;
        return false;
    }
}

// Change customer PIN
bool Customer::changePin(string oldPin, string newPin) {
    // TODO: PIN change logic
    // 1. First verify the old PIN using verifyPin()
    // 2. If old PIN is correct, validate new PIN (length, format)
    // 3. Hash the new PIN and store it
    // 4. Display success/failure message
    // 5. Return true if successful, false otherwise
    if (!verifyPin(oldPin)) {
        cout << "Error: Old PIN verification failed." << endl;
        return false;
    }
    if (newPin.empty() || newPin.length() < 4) {
        cout << "Error: New PIN must be at least 4 characters long." << endl;
        return false;
    }
    hashedPin = simpleHash(newPin);
    cout << "PIN changed successfully." << endl;
    cout << "New Hashed PIN: " << hashedPin << endl; 
    return true;  
}

// =============================================================================
// INFORMATION UPDATE FUNCTIONS
// =============================================================================

// Update contact information
void Customer::updateContactInfo(string newAddress, string newPhone, string newEmail) {
    if (!newEmail.empty() && !isValidEmail(newEmail)) {
        cout << "Error: Invalid email format." << endl;
        return;
    }
    if (!newPhone.empty() && !isValidPhone(newPhone)) {
        cout << "Error: Invalid phone number format." << endl;
        return;
    }
    address = newAddress;
    phoneNumber = newPhone;
    email = newEmail;
    cout << "Contact information updated successfully." << endl;
    cout << "New Address: " << address << endl;
    cout << "New Phone: " << phoneNumber << endl;
    cout << "New Email: " << email << endl;
    cout << "----------------------------------------" << endl;
}

// Set customer active status
void Customer::setActive(bool status) {
    isActive = status;
    if (isActive) {
        cout << "Customer with ID: " << customerId << " is now active." << endl;
    } else {
        cout << "Customer with ID: " << customerId << " is now inactive." << endl;
        for (Account* account : accounts) {
            account->setActive(false); // Assuming Account has setActive method 
        }
    cout << "All accounts for customer " << customerId << " have been deactivated." << endl;
    }
    cout << "----------------------------------------" << endl;
}

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================

// Display basic customer information
void Customer::displayCustomerInfo() const {
    cout << "Customer ID: " << customerId << endl;
    cout << "Name: " << name << endl;
    cout << "Address: " << address << endl;
    cout << "Phone: " << (phoneNumber.empty() ? "Not provided" : phoneNumber) << endl;
    cout << "Email: " << (email.empty() ? "Not provided" : email) << endl;
    cout << "Registration Date: " << registrationDate << endl;
    cout << "Status: " << (isActive ? "Active" : "Inactive") << endl;
    cout << "----------------------------------------" << endl; 
}

// Display account summary (brief info about all accounts)
void Customer::displayAccountSummary() const {
    cout << "Account Summary for Customer: " << name << " (ID: " << customerId << ")" << endl;
    cout << "----------------------------------------" << endl;
    cout << left << setw(20) << "Account Number"
         << left << setw(15) << "Account Type"
         << left << setw(15) << "Balance"
         << left << setw(10) << "Status" << endl;
    cout << "----------------------------------------" << endl;
    double totalBalance = 0.0;
    for (const Account* account : accounts) {
        cout << left << setw(20) << account->getAccountNumber()
             << left << setw(15) << (account->getAccountType() == AccountType::SAVINGS ? "SAVINGS" : "CHECKING")
             << left << setw(15) << fixed << setprecision(2) << account->getBalance()
             << left << setw(10) << (account->isAccountActive() ? "Active" : "Inactive") << endl;
        totalBalance += account->getBalance();
    }
    cout << "----------------------------------------" << endl;
    cout << "Total Balance: $" << fixed << setprecision(2) << totalBalance << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Active Accounts: " << getTotalActiveAccounts() << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Savings Balance: $" << fixed << setprecision(2) << getTotalSavingsBalance() << endl;
    cout << "Total Checking Balance: $" << fixed << setprecision(2) << getTotalCheckingBalance() << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Balance Across All Accounts: $" << fixed << setprecision(2) << getTotalBalance() << endl;
    cout << "----------------------------------------" << endl;
    cout << "End of Account Summary." << endl;
    cout << "----------------------------------------" << endl;
}

// Display detailed information for all accounts
void Customer::displayDetailedAccountInfo() const {
    cout << "Detailed Account Information for Customer: " << name << " (ID: " << customerId << ")" << endl;
    cout << "----------------------------------------" << endl;
    for (const Account* account : accounts) {
        account->displayAccountInfo();
        cout << "----------------------------------------" << endl;
    }
    if (accounts.empty()) {
        cout << "No accounts found for this customer." << endl;
    }
    cout << "----------------------------------------" << endl;
    cout << "End of Detailed Account Information." << endl;
    cout << "----------------------------------------" << endl;
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
    return totalBalance;
}

// Get total savings account balance
double Customer::getTotalSavingsBalance() const {
    vector<Account*> savingsAccounts = getAccountsByType(AccountType::SAVINGS);
    double totalSavingsBalance = 0.0;
    for (const Account* account : savingsAccounts) {
        totalSavingsBalance += account->getBalance();
    }
    return totalSavingsBalance;
}

// Get total checking account balance
double Customer::getTotalCheckingBalance() const {
    vector<Account*> checkingAccounts = getAccountsByType(AccountType::CHECKING);
    double totalCheckingBalance = 0.0;
    for (const Account* account : checkingAccounts) {
        totalCheckingBalance += account->getBalance();
    }
    return totalCheckingBalance;
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
