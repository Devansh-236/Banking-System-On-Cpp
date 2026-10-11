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
    LOAN_DISBURSEMENT,
    LOAN_PAYMENT,
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
    string linkedTransactionId;     // Other leg of a transfer, or the reversal pair

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
    string getDate() const;         // YYYY-MM-DD part of the timestamp
    string getDescription() const;
    double getBalanceBefore() const;
    double getBalanceAfter() const;
    TransactionStatus getStatus() const;
    string getCustomerId() const;
    string getSessionId() const;
    string getNotes() const;
    string getLinkedTransactionId() const;

    // Setters
    void setStatus(TransactionStatus newStatus);
    void setRelatedAccountNumber(string relatedAccNum);
    void setSessionId(string sessionId);
    void setNotes(string notes);
    void setBalanceAfter(double balance);
    void setTimestamp(string newTimestamp);
    void setLinkedTransactionId(string linkedId);

    // Utility functions
    string getTransactionTypeString() const;
    string getStatusString() const;
    bool isSuccessful() const;
    bool isTransfer() const;
    bool isCredit() const;          // Money flowing into the account
    bool affectsBalance() const;    // Completed or reversed (reversals have their own compensating entry)
    double getNetAmount() const;    // Positive for credits, negative for debits

    // Enum <-> string conversion (used for display and persistence)
    static string typeToString(TransactionType type);
    static TransactionType typeFromString(const string& name);
    static string statusToString(TransactionStatus status);
    static TransactionStatus statusFromString(const string& name);

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
    long long nextTransactionNumber;
    string logFilePath;
    string currentSessionId;                            // Stamped on new transactions

    void indexTransaction(const Transaction& transaction);
    void updateNextTransactionNumber(const string& transactionId);
    static bool inDateRange(const Transaction& txn, const string& startDate, const string& endDate);

public:
    // Constructor
    TransactionManager(string logFile = "data/transactions.dat");

    // Destructor
    ~TransactionManager() = default;

    // Session context
    void setSessionContext(string sessionId);
    string getSessionContext() const;

    // Core transaction processing (each returns the new transaction ID, or "" on failure)
    string processTransaction(TransactionType type, string accountNumber, double amount, string description,
                              double balanceBefore, double balanceAfter, string customerId,
                              string relatedAccount = "");
    string processDeposit(string accountNumber, double amount, string description,
                         double balanceBefore, double balanceAfter, string customerId);
    string processWithdrawal(string accountNumber, double amount, string description,
                            double balanceBefore, double balanceAfter, string customerId);
    string processTransfer(string fromAccount, string toAccount, double amount,
                          string description, double fromBalBefore, double fromBalAfter,
                          double toBalBefore, double toBalAfter, string fromCustomerId,
                          string toCustomerId = "");
    string processFeeCharge(string accountNumber, double amount, string description,
                           double balanceBefore, double balanceAfter, string customerId);
    string processInterestCredit(string accountNumber, double amount, string description,
                                double balanceBefore, double balanceAfter, string customerId);

    // Transaction management
    bool addTransaction(Transaction transaction);
    Transaction* findTransaction(string transactionId);
    const Transaction* findTransaction(string transactionId) const;
    bool updateTransactionStatus(string transactionId, TransactionStatus newStatus);
    bool reverseTransaction(string transactionId, string reason);   // Marks REVERSED; Bank moves the money
    bool linkTransactions(string firstId, string secondId);

    // History and search functions (all newest first unless noted)
    vector<Transaction> getAllTransactions() const;
    vector<Transaction> getAccountHistory(string accountNumber, int limit = 50) const;
    vector<Transaction> getCustomerHistory(string customerId, int limit = 100) const;
    vector<Transaction> getTransactionsByType(TransactionType type, int limit = 50) const;
    vector<Transaction> getTransactionsByDateRange(string startDate, string endDate) const;
    vector<Transaction> getAccountTransactionsByDateRange(string accountNumber, string startDate, string endDate) const;
    vector<Transaction> getTransactionsByAmountRange(double minAmount, double maxAmount) const;
    vector<Transaction> getFailedTransactions() const;
    vector<Transaction> getPendingTransactions() const;
    vector<Transaction> getLargeTransactions(double minAmount) const;
    const Transaction* getLatestAccountTransaction(string accountNumber) const;

    // Analytics functions (dates are YYYY-MM-DD, inclusive; empty means unbounded)
    double getTotalDeposits(string accountNumber, string startDate = "", string endDate = "") const;     // All credits
    double getTotalWithdrawals(string accountNumber, string startDate = "", string endDate = "") const;  // All debits
    double getNetFlow(string accountNumber, string startDate = "", string endDate = "") const;
    int getTransactionCount(string accountNumber, string startDate = "", string endDate = "") const;
    double getAverageTransactionAmount(string accountNumber, TransactionType type) const;
    double getDailyWithdrawalTotal(string accountNumber, string date) const;  // Cash withdrawals only

    // Display functions
    void displayTransactionHistory(string accountNumber, int limit = 20) const;
    void displayCustomerTransactionSummary(string customerId) const;
    void displayTransactionsByType(TransactionType type, int limit = 20) const;
    void displayDailyTransactionSummary(string date) const;
    void displayTransactionStatistics() const;
    static void displayTransactionTable(const vector<Transaction>& transactions);

    // Data persistence
    bool saveTransactionHistory() const;
    bool loadTransactionHistory();
    bool exportTransactionsToCSV(string filename, string accountNumber = "") const;
    bool createBackup(string backupPath = "backup/") const;
    void setLogFilePath(string path);
    string getLogFilePath() const;
    void clear();

    // Utility functions
    string generateTransactionId();
    bool isValidTransactionId(string transactionId) const;
    string getCurrentDate() const;
    string getCurrentDateTime() const;

    // System maintenance
    int cleanupOldTransactions(int daysOld = 365);
    void reindexTransactions();
    int getTotalTransactionCount() const;
    double getTotalSystemVolume() const;
};

#endif // TRANSACTION_H
