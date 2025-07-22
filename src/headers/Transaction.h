#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

// Transaction type enumeration
enum class TransactionType {
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER_IN,
    TRANSFER_OUT,
    FEE_CHARGE,
    INTEREST_CREDIT,
    ACCOUNT_OPENING,
    ACCOUNT_CLOSING,
    UNKNOWN
};

// Transaction status enumeration
enum class TransactionStatus {
    PENDING,
    COMPLETED,
    FAILED,
    CANCELLED,
    REVERSED
};

class Transaction {
private:
    string transactionId;
    string accountNumber;
    string relatedAccountNumber;    // For transfers
    TransactionType transactionType;
    double amount;
    string timestamp;
    string description;
    double balanceBefore;
    double balanceAfter;
    TransactionStatus status;
    string customerId;
    string sessionId;               // For tracking user sessions
    string notes;                   // Additional transaction notes

public:
    // Constructors
    Transaction();
    Transaction(string txnId, string accNum, TransactionType type, double amt, 
               string desc, double beforeBal, double afterBal, string custId);
    
    // Getters
    string getTransactionId() const;
    string getAccountNumber() const;
    string getRelatedAccountNumber() const;
    TransactionType getTransactionType() const;
    double getAmount() const;
    string getTimestamp() const;
    string getDescription() const;
    double getBalanceBefore() const;
    double getBalanceAfter() const;
    TransactionStatus getStatus() const;
    string getCustomerId() const;
    string getSessionId() const;
    string getNotes() const;
    
    // Setters
    void setStatus(TransactionStatus newStatus);
    void setRelatedAccountNumber(string relatedAccNum);
    void setSessionId(string sessionId);
    void setNotes(string notes);
    void setBalanceAfter(double balance);
    
    // Utility functions
    string getTransactionTypeString() const;
    string getStatusString() const;
    bool isSuccessful() const;
    bool isTransfer() const;
    double getNetAmount() const;  // Positive for credits, negative for debits
    
    // Display functions
    void displayTransaction() const;
    void displayTransactionSummary() const;
    
    // Comparison operators (for sorting)
    bool operator<(const Transaction& other) const;
    bool operator==(const Transaction& other) const;
};

// Transaction Manager class for handling transaction processing and history
class TransactionManager {
private:
    map<string, Transaction> transactionHistory;        // TransactionId -> Transaction
    multimap<string, string> accountTransactions;       // AccountNumber -> TransactionId
    multimap<string, string> customerTransactions;      // CustomerId -> TransactionId
    multimap<string, string> dateTransactions;          // Date -> TransactionId
    int nextTransactionNumber;
    string logFilePath;

public:
    // Constructor
    TransactionManager(string logFile = "data/transactions.log");
    
    // Destructor
    ~TransactionManager();
    
    // Core transaction processing
    string processDeposit(string accountNumber, double amount, string description, 
                         double balanceBefore, double balanceAfter, string customerId);
    string processWithdrawal(string accountNumber, double amount, string description,
                            double balanceBefore, double balanceAfter, string customerId);
    string processTransfer(string fromAccount, string toAccount, double amount,
                          string description, double fromBalBefore, double fromBalAfter,
                          double toBalBefore, double toBalAfter, string customerId);
    string processFeeCharge(string accountNumber, double amount, string description,
                           double balanceBefore, double balanceAfter, string customerId);
    string processInterestCredit(string accountNumber, double amount, string description,
                                double balanceBefore, double balanceAfter, string customerId);
    
    // Transaction management
    bool addTransaction(Transaction transaction);
    Transaction* findTransaction(string transactionId);
    bool updateTransactionStatus(string transactionId, TransactionStatus newStatus);
    bool reverseTransaction(string transactionId, string reason);
    
    // History and search functions
    vector<Transaction> getAccountHistory(string accountNumber, int limit = 50) const;
    vector<Transaction> getCustomerHistory(string customerId, int limit = 100) const;
    vector<Transaction> getTransactionsByType(TransactionType type, int limit = 50) const;
    vector<Transaction> getTransactionsByDateRange(string startDate, string endDate) const;
    vector<Transaction> getTransactionsByAmountRange(double minAmount, double maxAmount) const;
    vector<Transaction> getFailedTransactions() const;
    vector<Transaction> getPendingTransactions() const;
    
    // Analytics functions
    double getTotalDeposits(string accountNumber, string startDate = "", string endDate = "") const;
    double getTotalWithdrawals(string accountNumber, string startDate = "", string endDate = "") const;
    double getNetFlow(string accountNumber, string startDate = "", string endDate = "") const;
    int getTransactionCount(string accountNumber, string startDate = "", string endDate = "") const;
    double getAverageTransactionAmount(string accountNumber, TransactionType type) const;
    
    // Display functions
    void displayTransactionHistory(string accountNumber, int limit = 20) const;
    void displayCustomerTransactionSummary(string customerId) const;
    void displayTransactionsByType(TransactionType type, int limit = 20) const;
    void displayDailyTransactionSummary(string date) const;
    void displayTransactionStatistics() const;
    
    // Data persistence
    bool saveTransactionHistory() const;
    bool loadTransactionHistory();
    bool exportTransactionsToCSV(string filename, string accountNumber = "") const;
    bool createBackup(string backupPath = "backup/") const;
    
    // Utility functions
    string generateTransactionId();
    bool isValidTransactionId(string transactionId) const;
    string getCurrentDate() const;
    string getCurrentDateTime() const;
    
    // System maintenance
    void cleanupOldTransactions(int daysOld = 365);
    void reindexTransactions();
    int getTotalTransactionCount() const;
    double getTotalSystemVolume() const;
};

#endif // TRANSACTION_H
