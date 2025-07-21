#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <vector>
#include "Account.h"

using namespace std;

class Customer {
private:
    string customerId;
    string name;
    string address;
    string phoneNumber;
    string email;
    string hashedPin;           // Store hashed PIN for security
    vector<Account*> accounts;  // Vector of account pointers
    string registrationDate;
    bool isActive;

public:
    // Constructor
    Customer(string id, string customerName, string addr, string phone, string mail, string pin);
    
    // Destructor
    ~Customer();
    
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
    
    // Account management
    void addAccount(Account* account);
    bool removeAccount(string accountNumber);
    Account* findAccount(string accountNumber) const;
    bool hasAccount(string accountNumber) const;
    vector<Account*> getAccountsByType(AccountType type) const;
    
    // Authentication
    bool verifyPin(string inputPin) const;
    bool changePin(string oldPin, string newPin);
    
    // Information update
    void updateContactInfo(string newAddress, string newPhone, string newEmail);
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
