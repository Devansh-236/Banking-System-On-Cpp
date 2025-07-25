#ifndef BANK_H
#define BANK_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Account.h"
#include "Customer.h"
#include "Transaction.h"
#include "Utilities.h"

using namespace std;

class Bank {
private:
    string bankName;
    string bankCode;
    map<string, unique_ptr<Customer>> customers;        // CustomerId -> Customer 
    map<string, Account*> accounts;                     // AccountNumber -> Account  
    unique_ptr<TransactionManager> transactionManager;
    map<string, string> accountCustomerMap;             // AccountNumber -> CustomerId
    
    // System counters
    int nextCustomerNumber;
    int nextSavingsAccountNumber;
    int nextCheckingAccountNumber;
    
    // System configuration
    double minSavingsBalance;
    double minCheckingBalance;
    double defaultOverdraftLimit;
    double savingsInterestRate;
    
    // System statistics
    double totalDeposits;
    double totalWithdrawals;
    int totalCustomersCreated;
    int totalAccountsCreated;

public:
    // Constructor and Destructor
    Bank(string name, string code = "BANK001");
    ~Bank();
    
    // Customer Management
    string createCustomer(string name, string address, string phone, 
                         string email, string pin);
    bool deleteCustomer(string customerId);
    Customer* findCustomer(string customerId) const;
    bool authenticateCustomer(string customerId, string pin) const;
    vector<Customer*> getAllCustomers() const;
    bool updateCustomerInfo(string customerId, string newAddress, 
                           string newPhone, string newEmail);
    
    // Account Management
    string createSavingsAccount(string customerId, double initialDeposit);
    string createCheckingAccount(string customerId, double initialDeposit, 
                                double overdraftLimit = 0);
    bool closeAccount(string accountNumber);
    Account* findAccount(string accountNumber) const;
    vector<Account*> getCustomerAccounts(string customerId) const;
    bool transferAccountOwnership(string accountNumber, string newCustomerId);
    
    // Transaction Operations
    bool deposit(string accountNumber, double amount, string description = "Deposit");
    bool withdraw(string accountNumber, double amount, string description = "Withdrawal");
    bool transfer(string fromAccount, string toAccount, double amount, 
                 string description = "Transfer");
    bool applyInterestToSavingsAccounts();
    bool chargeMonthlyFees();
    
    // Account Services
    bool freezeAccount(string accountNumber, string reason);
    bool unfreezeAccount(string accountNumber);
    bool changeAccountType(string accountNumber, AccountType newType);
    double getAccountBalance(string accountNumber) const;
    
    // Transaction History & Analytics
    vector<Transaction> getAccountTransactionHistory(string accountNumber, 
                                                   int limit = 50) const;
    vector<Transaction> getCustomerTransactionHistory(string customerId, 
                                                    int limit = 100) const;
    double getAccountNetFlow(string accountNumber, string startDate = "", 
                           string endDate = "") const;
    
    // System Administration
    void displaySystemStatistics() const;
    void displayAllCustomers() const;
    void displayAllAccounts() const;
    void displayLargeTransactions(double minAmount) const;
    void displayInactiveAccounts() const;
    void displayCustomersWithMultipleAccounts() const;
    
    // System Maintenance
    void performSystemMaintenance();
    void generateDailyReport(string date) const;
    void generateMonthlyReport(string month) const;
    bool backupSystemData() const;
    bool restoreSystemData() const;
    
    // System Configuration
    void setMinimumBalances(double savingsMin, double checkingMin);
    void setDefaultOverdraftLimit(double limit);
    void setSavingsInterestRate(double rate);
    void displaySystemConfiguration() const;
    
    // Data Persistence
    bool saveAllData() const;
    bool loadAllData();
    
    // System Validation
    bool validateSystemIntegrity() const;
    void fixDataInconsistencies();
    
    // Getters
    string getBankName() const;
    string getBankCode() const;
    int getTotalCustomers() const;
    int getTotalAccounts() const;
    double getTotalSystemBalance() const;
    int getTotalTransactions() const;
    
    // ID Generation
    string generateCustomerId();
    string generateSavingsAccountNumber();
    string generateCheckingAccountNumber();
};

#endif // BANK_H
