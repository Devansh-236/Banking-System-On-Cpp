#ifndef BANK_H
#define BANK_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Account.h"
#include "Customer.h"
#include "Transaction.h"
#include "Loan.h"
#include "FileManager.h"
#include "Utilities.h"

using namespace std;

// The Bank owns every customer, account and loan, enforces business rules,
// records every money movement through the TransactionManager, and persists
// state through the FileManager. Operations never prompt or print on failure:
// they return false/"" and set getLastError() for the UI to show.
class Bank {
private:
    string bankName;
    string bankCode;
    map<string, unique_ptr<Customer>> customers;        // CustomerId -> Customer
    map<string, unique_ptr<Account>> accounts;          // AccountNumber -> Account (owner)
    map<string, unique_ptr<Loan>> loans;                // LoanId -> Loan
    unique_ptr<TransactionManager> transactionManager;
    map<string, string> accountCustomerMap;             // AccountNumber -> CustomerId
    FileManager fileManager;
    string configFilePath;
    bool autoSave;

    // System counters
    int nextCustomerNumber;
    int nextSavingsAccountNumber;
    int nextCheckingAccountNumber;
    int nextLoanNumber;

    // System configuration
    double minSavingsBalance;
    double minCheckingBalance;
    double defaultOverdraftLimit;
    double savingsInterestRate;
    double loanInterestRate;
    double maxDailyWithdrawal;
    double maxTransferAmount;
    double checkingTransactionFee;
    double maxLoanAmount;
    int maxSavingsWithdrawalsPerMonth;
    int freeCheckingTransactions;
    int maxActiveLoansPerCustomer;
    int maxLoginAttempts;
    int sessionTimeoutSeconds;
    int backupIntervalHours;
    int logRetentionDays;
    string lastMonthEndProcessed;                       // YYYY-MM of the last month-end run

    // System statistics
    double totalDeposits;
    double totalWithdrawals;
    int totalCustomersCreated;
    int totalAccountsCreated;

    mutable string lastError;

    // Internal helpers
    bool fail(const string& message) const;             // Sets lastError and returns false
    string failId(const string& message) const;         // Sets lastError and returns ""
    void persist();                                     // Saves if autoSave is enabled
    string ownerOf(const string& accountNumber) const;
    Account* requireAccount(const string& accountNumber, bool mustBeActive);
    vector<Loan*> activeLoansForAccount(const string& accountNumber) const;
    double lowestBalanceForAdminDebit(const Account* account) const;
    void registerAccount(unique_ptr<Account> account, Customer* owner);
    void removeAccountRecord(const string& accountNumber);
    void syncCountersWithData();
    void displayReportHeader(const string& title) const;

public:
    // Constructor and Destructor
    Bank(string name, string code = "BANK001", string dataDir = "data",
         string backupDir = "backup", string logDir = "logs");
    ~Bank();

    // Error reporting
    string getLastError() const;

    // Customer Management
    string createCustomer(string name, string address, string phone,
                         string email, string pin);
    bool deleteCustomer(string customerId);
    Customer* findCustomer(string customerId) const;
    Customer* findCustomerByEmail(string email) const;
    bool authenticateCustomer(string customerId, string pin) const;
    vector<Customer*> getAllCustomers() const;
    bool updateCustomerInfo(string customerId, string newAddress,
                           string newPhone, string newEmail);
    bool changeCustomerPin(string customerId, string oldPin, string newPin);
    bool resetCustomerPin(string customerId, string newPin);
    bool setCustomerActive(string customerId, bool active);
    bool unlockCustomer(string customerId);

    // Account Management
    string createSavingsAccount(string customerId, double initialDeposit);
    string createCheckingAccount(string customerId, double initialDeposit,
                                double overdraftLimit = -1);  // negative = bank default
    bool closeAccount(string accountNumber, string transferToAccount = "",
                      bool adminOverride = false);    // Admins may close frozen accounts
    Account* findAccount(string accountNumber) const;
    vector<Account*> getCustomerAccounts(string customerId) const;
    bool customerOwnsAccount(string customerId, string accountNumber) const;
    bool transferAccountOwnership(string accountNumber, string newCustomerId);

    // Transaction Operations
    bool deposit(string accountNumber, double amount, string description = "Deposit");
    bool withdraw(string accountNumber, double amount, string description = "Withdrawal");
    bool transfer(string fromAccount, string toAccount, double amount,
                 string description = "Transfer");
    bool applyInterestToSavingsAccounts();
    void chargeMonthlyFees();
    double accrueLoanInterest();
    bool processMonthEnd(bool force = false);
    bool reverseTransaction(string transactionId, string reason);

    // Account Services
    bool freezeAccount(string accountNumber, string reason);
    bool unfreezeAccount(string accountNumber);
    string changeAccountType(string accountNumber, AccountType newType);   // Returns new account number
    bool setAccountOverdraftLimit(string accountNumber, double limit);
    double getAccountBalance(string accountNumber) const;

    // Loans
    string applyForLoan(string customerId, string accountNumber, double amount, int termMonths);
    bool makeLoanPayment(string loanId, double amount);
    Loan* findLoan(string loanId) const;
    vector<Loan*> getCustomerLoans(string customerId) const;
    vector<Loan*> getAllLoans() const;
    double getCustomerOutstandingLoans(string customerId) const;
    double getTotalOutstandingLoans() const;
    void displayAllLoans() const;
    void displayCustomerLoans(string customerId) const;

    // Transaction History & Analytics
    vector<Transaction> getAccountTransactionHistory(string accountNumber,
                                                   int limit = 50) const;
    vector<Transaction> getCustomerTransactionHistory(string customerId,
                                                    int limit = 100) const;
    double getAccountNetFlow(string accountNumber, string startDate = "",
                           string endDate = "") const;
    bool exportAccountStatement(string accountNumber, string filename) const;
    const TransactionManager& getTransactionManager() const;
    void setSessionContext(string sessionId);

    // System Administration
    void displaySystemStatistics() const;
    void displayAllCustomers() const;
    void displayAllAccounts() const;
    void displayCustomerDetails(string customerId) const;
    void displayLargeTransactions(double minAmount) const;
    void displayInactiveAccounts() const;
    void displayCustomersWithMultipleAccounts() const;

    // System Maintenance
    void performSystemMaintenance();
    void generateDailyReport(string date) const;
    void generateMonthlyReport(string month) const;
    bool backupSystemData(string& backupName) const;
    bool restoreSystemData(string backupName = "");   // Empty = newest backup
    vector<string> listBackups() const;

    // System Configuration
    bool loadConfiguration(string path);
    bool saveConfiguration() const;
    bool setMinimumBalances(double savingsMin, double checkingMin);
    bool setDefaultOverdraftLimit(double limit);
    bool setSavingsInterestRate(double rate);
    bool setLoanInterestRate(double rate);
    bool setTransactionLimits(double dailyWithdrawal, double transferAmount);
    void displaySystemConfiguration() const;
    void setLogRetentionDays(int days);
    int getMaxLoginAttempts() const;
    int getSessionTimeout() const;
    int getBackupInterval() const;
    double getMinSavingsBalance() const;
    double getMinCheckingBalance() const;
    double getDefaultOverdraftLimit() const;
    double getLoanInterestRate() const;
    double getMaxLoanAmount() const;

    // Data Persistence
    bool saveAllData() const;
    bool loadAllData();
    void setAutoSave(bool enabled);
    FileManager& getFileManager();

    // System Validation
    vector<string> findIntegrityIssues() const;
    bool validateSystemIntegrity() const;
    int fixDataInconsistencies();

    // Getters
    string getBankName() const;
    string getBankCode() const;
    int getTotalCustomers() const;
    int getTotalAccounts() const;
    double getTotalSystemBalance() const;
    int getTotalTransactions() const;
    int getTotalActiveAccounts() const;
    int getTotalInactiveAccounts() const;
    int getSavingsAccountCount() const;
    int getCheckingAccountCount() const;
    int getActiveCustomerCount() const;
    int getInactiveCustomerCount() const;

    // ID Generation
    string generateCustomerId();
    string generateSavingsAccountNumber();
    string generateCheckingAccountNumber();
    string generateLoanId();
};

#endif // BANK_H
