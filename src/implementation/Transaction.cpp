#include "Transaction.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace std;

// =============================================================================
// TRANSACTION CLASS IMPLEMENTATIONS
// =============================================================================

// Default constructor
Transaction::Transaction()
{
    transactionId = "";
    accountNumber = "";
    relatedAccountNumber = "";
    transactionType = TransactionType::UNKNOWN;
    amount = 0.0;
    timestamp = getCurrentDateTime();
    description = "";
    balanceBefore = 0.0;
    balanceAfter = 0.0;
    status = TransactionStatus::PENDING;
    customerId = "";
    sessionId = "";
    notes = "";
}

// Parameterized constructor
Transaction::Transaction(string txnId, string accNum, TransactionType type, double amt,
                         string desc, double beforeBal, double afterBal, string custId)
{
    transactionId = txnId;
    accountNumber = accNum;
    relatedAccountNumber = "";
    transactionType = type;
    amount = amt;
    timestamp = getCurrentDateTime();
    description = desc;
    balanceBefore = beforeBal;
    balanceAfter = afterBal;
    status = TransactionStatus::COMPLETED; // Default to completed
    customerId = custId;
    sessionId = ""; // Set to empty by default
    notes = "";     // Set to empty by default
}

// Get transaction ID
string Transaction::getTransactionId() const
{
    return transactionId;
}

// Get account number
string Transaction::getAccountNumber() const
{
    return accountNumber;
}

// Get related account number (for transfers)
string Transaction::getRelatedAccountNumber() const
{
    if (relatedAccountNumber.empty())
    {
        return "N/A"; // Return "N/A" if not set
    }
    return relatedAccountNumber;
}

// Get transaction type
TransactionType Transaction::getTransactionType() const
{
    return transactionType;
}

// Get transaction amount
double Transaction::getAmount() const
{
    return amount;
}

// Get timestamp
string Transaction::getTimestamp() const
{
    return timestamp;
}

// Get description
string Transaction::getDescription() const
{
    if (description.empty())
    {
        return "No description provided"; // Default message if empty
    }
    return description;
}

// Get balance before transaction
double Transaction::getBalanceBefore() const
{
    return balanceBefore;
}

// Get balance after transaction
double Transaction::getBalanceAfter() const
{
    if (balanceAfter == 0.0)
    {
        return balanceBefore + amount; // Default calculation if not set
    }
    return balanceAfter;
}

// Get transaction status
TransactionStatus Transaction::getStatus() const
{
    return status;
}

// Get customer ID
string Transaction::getCustomerId() const
{
    if (customerId.empty())
    {
        return "N/A"; // Return "N/A" if not set
    }
    return customerId;
}

// Get session ID
string Transaction::getSessionId() const
{
    if (sessionId.empty())
    {
        return "N/A"; // Return "N/A" if not set
    }
    return sessionId;
}

// Get notes
string Transaction::getNotes() const
{
    if (notes.empty())
    {
        return "No notes provided"; // Default message if empty
    }
    return notes;
}

// Set transaction status
void Transaction::setStatus(TransactionStatus newStatus)
{
    if (status != newStatus)
    {
        status = newStatus;
        switch (newStatus)
        {
        case TransactionStatus::PENDING:
            cout << "Transaction " << transactionId << " is now pending." << endl;
            break;
        case TransactionStatus::COMPLETED:
            cout << "Transaction " << transactionId << " has been completed successfully." << endl;
            break;
        case TransactionStatus::FAILED:
            cout << "Transaction " << transactionId << " has failed." << endl;
            break;
        default:
            cout << "Transaction " << transactionId << " status updated." << endl;
        }
    }
}

// Set related account number
void Transaction::setRelatedAccountNumber(string relatedAccNum)
{
    if (!relatedAccNum.empty())
    {
        relatedAccountNumber = relatedAccNum;
    }
    else
    {
        relatedAccountNumber = ""; // Reset if empty
    }
}

// Set session ID
void Transaction::setSessionId(string sessionId)
{
    if (!sessionId.empty())
    {
        this->sessionId = sessionId;
    }
    else
    {
        this->sessionId = ""; // Reset if empty
    }
}

// Set notes
void Transaction::setNotes(string notes)
{
    if (!notes.empty())
    {
        this->notes = notes;
    }
    else
    {
        this->notes = ""; // Reset if empty
    }
}

// Set balance after transaction
void Transaction::setBalanceAfter(double balance)
{
    if (balance >= 0)
    {
        balanceAfter = balance;
    }
    else
    {
        cout << "Invalid balance after transaction. Must be non-negative." << endl;
    }
}

// Get transaction type as string
string Transaction::getTransactionTypeString() const
{
    switch (transactionType)
    {
    case TransactionType::DEPOSIT:
        return "DEPOSIT";
    case TransactionType::WITHDRAWAL:
        return "WITHDRAWAL";
    case TransactionType::TRANSFER_IN:
        return "TRANSFER_IN";
    case TransactionType::TRANSFER_OUT:
        return "TRANSFER_OUT";
    case TransactionType::FEE_CHARGE:
        return "FEE_CHARGE";
    case TransactionType::INTEREST_CREDIT:
        return "INTEREST_CREDIT";
    default:
        return "UNKNOWN";
    }
}

// Get status as string
string Transaction::getStatusString() const
{
    switch (status)
    {
    case TransactionStatus::PENDING:
        return "PENDING";
    case TransactionStatus::COMPLETED:
        return "COMPLETED";
    case TransactionStatus::FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}

// Check if transaction was successful
bool Transaction::isSuccessful() const
{
    return status == TransactionStatus::COMPLETED;
}

// Check if transaction is a transfer
bool Transaction::isTransfer() const
{
    return transactionType == TransactionType::TRANSFER_IN || transactionType == TransactionType::TRANSFER_OUT;
}

// Get net amount (positive for credits, negative for debits)
double Transaction::getNetAmount() const
{
    switch (transactionType)
    {
    case TransactionType::DEPOSIT:
    case TransactionType::TRANSFER_IN:
    case TransactionType::INTEREST_CREDIT:
        return amount; // Positive for credits
    case TransactionType::WITHDRAWAL:
    case TransactionType::TRANSFER_OUT:
    case TransactionType::FEE_CHARGE:
        return -amount; // Negative for debits
    default:
        return 0.0; // Unknown type, no net amount
    }
}

// Display detailed transaction information
void Transaction::displayTransaction() const
{
    cout << "Transaction ID: " << transactionId << endl;
    cout << "Account Number: " << accountNumber << endl;
    cout << "Related Account: " << (relatedAccountNumber.empty() ? "N/A" : relatedAccountNumber) << endl;
    cout << "Type: " << getTransactionTypeString() << endl;
    cout << "Amount: " << fixed << setprecision(2) << amount << endl;
    cout << "Timestamp: " << timestamp << endl;
    cout << "Description: " << (description.empty() ? "No description provided" : description) << endl;
    cout << "Balance Before: " << fixed << setprecision(2) << balanceBefore << endl;
    cout << "Balance After: " << fixed << setprecision(2) << balanceAfter << endl;
    cout << "Status: " << getStatusString() << endl;
    cout << "Customer ID: " << (customerId.empty() ? "N/A" : customerId) << endl;
    cout << "Session ID: " << (sessionId.empty() ? "N/A" : sessionId) << endl;
    cout << "Notes: " << (notes.empty() ? "No notes provided" : notes) << endl;
    cout << "----------------------------------------" << endl;
    cout << "Net Amount: " << fixed << setprecision(2) << getNetAmount() << endl;
    cout << "Successful: " << (isSuccessful() ? "Yes" : "No") << endl;
    cout << "Is Transfer: " << (isTransfer() ? "Yes" : "No") << endl;
    cout << "----------------------------------------" << endl;
    cout << "End of Transaction Details" << endl;
    cout << "----------------------------------------" << endl;
}

// Display transaction summary (one line)
void Transaction::displayTransactionSummary() const
{
    cout << "ID: " << transactionId
         << " | Type: " << getTransactionTypeString()
         << " | Amount: " << fixed << setprecision(2) << amount
         << " | Status: " << getStatusString()
         << " | Date: " << timestamp
         << endl;
}

// Comparison operator for sorting
bool Transaction::operator<(const Transaction &other) const
{
    return timestamp < other.timestamp;
}

// Equality operator
bool Transaction::operator==(const Transaction &other) const
{
    return transactionId == other.transactionId;
}

// =============================================================================
// TRANSACTION MANAGER CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor
TransactionManager::TransactionManager(string logFile)
{
    logFilePath = logFile;
    // Initialize empty maps and multimaps
    transactionHistory.clear();
    accountTransactions.clear();
    customerTransactions.clear();
    dateTransactions.clear();
    nextTransactionNumber = 1; // Start with transaction number 1
    if (!loadTransactionHistory())
    {
        cout << "No existing transaction history found. Starting fresh." << endl;
    }
    else
    {
        cout << "Transaction history loaded successfully." << endl;
        cout << "Transaction Manager initialized with log file: " << logFilePath << endl;
    }
    cout << "Next transaction number: " << nextTransactionNumber << endl;
    cout << "----------------------------------------" << endl;
}

// Destructor
TransactionManager::~TransactionManager()
{
    if (!saveTransactionHistory())
    {
        cout << "Failed to save transaction history on exit." << endl;
    }
    else
    {
        cout << "Transaction history saved successfully on exit." << endl;
    }
    cout << "Transaction Manager destroyed." << endl;
}

// Process deposit transaction
string TransactionManager::processDeposit(string accountNumber, double amount, string description,
                                          double balanceBefore, double balanceAfter, string customerId)
{
    string transactionId = generateTransactionId();
    Transaction transaction(transactionId, accountNumber, TransactionType::DEPOSIT, amount,
                            description, balanceBefore, balanceAfter, customerId);
    transaction.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (addTransaction(transaction))
    {
        cout << "Deposit transaction processed successfully: " << transactionId << endl;
        return transactionId; // Return the transaction ID
    }
    else
    {
        cout << "Failed to process deposit transaction." << endl;
        return ""; // Return empty string on failure
    }
}

// Process withdrawal transaction
string TransactionManager::processWithdrawal(string accountNumber, double amount, string description,
                                             double balanceBefore, double balanceAfter, string customerId)
{
    string transactionId = generateTransactionId();
    Transaction transaction(transactionId, accountNumber, TransactionType::WITHDRAWAL, amount,
                            description, balanceBefore, balanceAfter, customerId);
    transaction.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (addTransaction(transaction))
    {
        cout << "Withdrawal transaction processed successfully: " << transactionId << endl;
        return transactionId; // Return the transaction ID
    }
    else
    {
        cout << "Failed to process withdrawal transaction." << endl;
        return ""; // Return empty string on failure
    }
}

// Process transfer transaction
string TransactionManager::processTransfer(string fromAccount, string toAccount, double amount,
                                           string description, double fromBalBefore, double fromBalAfter,
                                           double toBalBefore, double toBalAfter, string customerId)
{
    string transferOutId = generateTransactionId();
    Transaction transferOut(transferOutId, fromAccount, TransactionType::TRANSFER_OUT, amount,
                            description, fromBalBefore, fromBalAfter, customerId);
    transferOut.setRelatedAccountNumber(toAccount);
    transferOut.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (!addTransaction(transferOut))
    {
        cout << "Failed to process transfer OUT transaction." << endl;
        return ""; // Return empty string on failure
    }
    cout << "Transfer OUT transaction processed successfully: " << transferOutId << endl;
    // Now create the transfer IN transaction
    string transferInId = generateTransactionId();
    Transaction transferIn(transferInId, toAccount, TransactionType::TRANSFER_IN, amount,
                           description, toBalBefore, toBalAfter, customerId);
    transferIn.setRelatedAccountNumber(fromAccount);
    transferIn.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (!addTransaction(transferIn))
    {
        cout << "Failed to process transfer IN transaction." << endl;
        // Rollback the transfer OUT transaction
        updateTransactionStatus(transferOutId, TransactionStatus::FAILED);
        cout << "Transfer OUT transaction rolled back due to failure in transfer IN." << endl;
        return ""; // Return empty string on failure
    }
    cout << "Transfer IN transaction processed successfully: " << transferInId << endl;
    return transferOutId; // Return the transfer OUT transaction ID
}
// Update transaction status
bool TransactionManager::updateTransactionStatus(string transactionId, TransactionStatus newStatus)
{
    auto it = transactionHistory.find(transactionId);
    if (it != transactionHistory.end())
    {
        Transaction &transaction = it->second;
        TransactionStatus oldStatus = transaction.getStatus();
        transaction.setStatus(newStatus);
        cout << "Transaction " << transactionId << " status changed from "
             << static_cast<int>(oldStatus) << " to " << static_cast<int>(newStatus)
             << " at " << getCurrentDateTime() << endl;
        // Log the status change
        ofstream logFile(logFilePath, ios::app);
        if (logFile.is_open())
        {
            logFile << "Transaction " << transactionId << " status changed from "
                    << static_cast<int>(oldStatus) << " to " << static_cast<int>(newStatus)
                    << " at " << getCurrentDateTime() << endl;
            logFile.close();
        }
        else
        {
            cout << "Failed to open log file for writing." << endl;
        }
        // Update the transaction in the history
        transactionHistory[transactionId] = transaction;
        // Update the status in the accountTransactions, customerTransactions, and dateTransactions maps
        auto range = accountTransactions.equal_range(transaction.getAccountNumber());
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second == transactionId)
            {
                accountTransactions.erase(it);
                accountTransactions.emplace(transaction.getAccountNumber(), transactionId);
                break; // Exit loop after updating
            }
        }
        range = customerTransactions.equal_range(transaction.getCustomerId());
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second == transactionId)
            {
                customerTransactions.erase(it);
                customerTransactions.emplace(transaction.getCustomerId(), transactionId);
                break; // Exit loop after updating
            }
        }
        range = dateTransactions.equal_range(transaction.getTimestamp().substr(0, 10)); // Use date part
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second == transactionId)
            {
                dateTransactions.erase(it);
                dateTransactions.emplace(transaction.getTimestamp().substr(0, 10), transactionId);
                break; // Exit loop after updating
            }
        }
        cout << "Transaction status updated successfully." << endl;
        return true; // Successfully updated status
    }
    else
    {
        cout << "Transaction with ID " << transactionId << " not found." << endl;
        return false; // Transaction not found
    }
    cout << "Transaction status update failed." << endl;
    return false; // Failed to update status
}

// Reverse a transaction
bool TransactionManager::reverseTransaction(string transactionId, string reason)
{
    auto it = transactionHistory.find(transactionId);
    if (it != transactionHistory.end())
    {
        Transaction &originalTransaction = it->second;
        if (originalTransaction.getStatus() != TransactionStatus::COMPLETED)
        {
            cout << "Transaction " << transactionId << " cannot be reversed. Current status: "
                 << originalTransaction.getStatusString() << endl;
            return false; // Cannot reverse non-completed transactions
        }
        // Set original transaction status to REVERSED
        originalTransaction.setStatus(TransactionStatus::REVERSED);
        originalTransaction.setNotes("Reversed: " + reason);
        cout << "Transaction " << transactionId << " has been reversed." << endl;
        // Determine reversal type based on original transaction type
        TransactionType reversalType;
        if (originalTransaction.getTransactionType() == TransactionType::DEPOSIT)
        {
            reversalType = TransactionType::WITHDRAWAL; // Reversal for deposit is a withdrawal
        }
        else if (originalTransaction.getTransactionType() == TransactionType::WITHDRAWAL)
        {
            reversalType = TransactionType::DEPOSIT; // Reversal for withdrawal is a deposit
        }
        else if (originalTransaction.isTransfer())
        {
            // For transfers, create a new transfer transaction with opposite direction
            string relatedAccount = originalTransaction.getRelatedAccountNumber();
            string fromAccount = originalTransaction.getAccountNumber();
            string toAccount = relatedAccount;
            double amount = originalTransaction.getAmount();
            string description = "Reversal of transfer " + originalTransaction.getTransactionId();
            double fromBalBefore = originalTransaction.getBalanceBefore();
            double fromBalAfter = fromBalBefore + amount; // Reversal adds back the amount
            double toBalBefore = originalTransaction.getBalanceAfter();
            double toBalAfter = toBalBefore - amount; // Reversal deducts the amount
            return (processTransfer(fromAccount, toAccount, amount, description,
                                    fromBalBefore, fromBalAfter, toBalBefore, toBalAfter,
                                    originalTransaction.getCustomerId()) == "");
        }
        else
        {
            cout << "Reversal not implemented for transaction type: "
                 << originalTransaction.getTransactionTypeString() << endl;
            return false; // Reversal not implemented for this type
        }
        // Create reversal transaction
        string reversalId = generateTransactionId();
        Transaction reversalTransaction(reversalId, originalTransaction.getAccountNumber(),
                                        reversalType, originalTransaction.getAmount(),
                                        "Reversal of " + originalTransaction.getTransactionId(),
                                        originalTransaction.getBalanceBefore(),
                                        originalTransaction.getBalanceAfter(),
                                        originalTransaction.getCustomerId());
        reversalTransaction.setRelatedAccountNumber(originalTransaction.getRelatedAccountNumber());
        reversalTransaction.setStatus(TransactionStatus::COMPLETED); // Set status to completed
        if (addTransaction(reversalTransaction))
        {
            cout << "Reversal transaction created successfully: " << reversalId << endl;
            return true; // Successfully created reversal transaction
        }
        else
        {
            cout << "Failed to create reversal transaction." << endl;
            return false; // Failed to add reversal transaction
        }
    }
    else
    {
        cout << "Transaction with ID " << transactionId << " not found." << endl;
        return false; // Transaction not found
    }
}

// Add transaction to system
bool TransactionManager::addTransaction(Transaction transaction)
{
    string transactionId = transaction.getTransactionId();
    if (transactionHistory.find(transactionId) != transactionHistory.end())
    {
        cout << "Transaction with ID " << transactionId << " already exists." << endl;
        return false; // Transaction already exists
    }
    // Add to transaction history
    transactionHistory[transactionId] = transaction;
    // Add to account transactions
    accountTransactions.emplace(transaction.getAccountNumber(), transactionId);
    // Add to customer transactions
    transaction.setSessionId(""); // Reset session ID if not set
    customerTransactions.emplace(transaction.getCustomerId(), transactionId);
    // Add to date transactions
    string date = transaction.getTimestamp().substr(0, 10); // Use date part
    dateTransactions.emplace(date, transactionId);
    // Update next transaction number if necessary
    size_t lastDash = transactionId.find_last_of('-');
    if (lastDash != string::npos && lastDash + 1 < transactionId.length())
    {
        string numericPart = transactionId.substr(lastDash + 1);
        int txnNumber = stoi(numericPart);
        if (nextTransactionNumber <= txnNumber)
        {
            nextTransactionNumber = txnNumber + 1;
        }
    }
    cout << "Transaction " << transactionId << " added successfully." << endl;
    return true; // Successfully added
}

// Find transaction by ID
Transaction *TransactionManager::findTransaction(string transactionId)
{
    auto it = transactionHistory.find(transactionId);
    if (it != transactionHistory.end())
    {
        return &it->second; // Return pointer to the found transaction
    }
    else
    {
        cout << "Transaction with ID " << transactionId << " not found." << endl;
        return nullptr; // Transaction not found
    }
}

// Get account transaction history
vector<Transaction> TransactionManager::getAccountHistory(string accountNumber, int limit) const
{
    vector<Transaction> history;
    auto range = accountTransactions.equal_range(accountNumber);
    for (auto it = range.first; it != range.second && history.size() < static_cast<size_t>(limit); ++it)
    {
        const string &transactionId = it->second;
        auto txnIt = transactionHistory.find(transactionId);
        if (txnIt != transactionHistory.end())
        {
            history.push_back(txnIt->second); // Add transaction to history
        }
    }
    // Sort history by timestamp (newest first)
    sort(history.begin(), history.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() > b.getTimestamp(); // Newest first
         });
    cout << "Retrieved " << history.size() << " transactions for account " << accountNumber << "." << endl;
    return history; // Return the transaction history
}

// Get customer transaction history
vector<Transaction> TransactionManager::getCustomerHistory(string customerId, int limit) const
{
    vector<Transaction> history;
    auto range = customerTransactions.equal_range(customerId);
    for (auto it = range.first; it != range.second && history.size() < static_cast<size_t>(limit); ++it)
    {
        const string &transactionId = it->second;
        auto txnIt = transactionHistory.find(transactionId);
        if (txnIt != transactionHistory.end())
        {
            history.push_back(txnIt->second); // Add transaction to history
        }
    }
    // Sort history by timestamp (newest first)
    sort(history.begin(), history.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() > b.getTimestamp(); // Newest first
         });
    cout << "Retrieved " << history.size() << " transactions for customer " << customerId << "." << endl;
    return history; // Return the transaction history
}

// Get transactions by type
vector<Transaction> TransactionManager::getTransactionsByType(TransactionType type, int limit) const
{
    vector<Transaction> transactions;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        if (txn.getTransactionType() == type)
        {
            transactions.push_back(txn);
            if (transactions.size() >= static_cast<size_t>(limit))
            {
                break; // Limit reached
            }
        }
    }
    // Sort transactions by timestamp (newest first)
    sort(transactions.begin(), transactions.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() > b.getTimestamp(); // Newest first
         });
    cout << "Retrieved " << transactions.size() << " transactions of type "
         << static_cast<int>(type) << "." << endl;
    return transactions; // Return the filtered transactions
}

// Display transaction history for account
void TransactionManager::displayTransactionHistory(string accountNumber, int limit) const
{
    vector<Transaction> history = getAccountHistory(accountNumber, limit);
    cout << "Transaction History for Account: " << accountNumber << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << left << setw(15) << "Transaction ID"
         << setw(15) << "Type"
         << setw(10) << "Amount"
         << setw(20) << "Timestamp"
         << setw(20) << "Status"
         << setw(20) << "Description" << endl;
    cout << "--------------------------------------------------------" << endl;
    for (const Transaction &txn : history)
    {
        cout << left << setw(15) << txn.getTransactionId()
             << setw(15) << txn.getTransactionTypeString()
             << setw(10) << fixed << setprecision(2) << txn.getAmount()
             << setw(20) << txn.getTimestamp()
             << setw(20) << txn.getStatusString()
             << setw(20) << txn.getDescription() << endl;
    }
    cout << "--------------------------------------------------------" << endl;
    cout << "Total Transactions: " << history.size() << endl;
    double netFlow = 0.0;
    for (const Transaction &txn : history)
    {
        netFlow += txn.getNetAmount(); // Calculate net flow
    }
    cout << "Net Flow: " << fixed << setprecision(2) << netFlow << endl;
    cout << "--------------------------------------------------------" << endl;
}

// Get total deposits for account
double TransactionManager::getTotalDeposits(string accountNumber, string startDate, string endDate) const
{
    double totalDeposits = 0.0;
    vector<Transaction> history = getAccountHistory(accountNumber, INT_MAX); // Get all transactions
    for (const Transaction &txn : history)
    {
        if (txn.getTransactionType() == TransactionType::DEPOSIT)
        {
            if (startDate.empty() || endDate.empty() ||
                (txn.getTimestamp() >= startDate && txn.getTimestamp() <= endDate))
            {
                totalDeposits += txn.getAmount(); // Add to total deposits
            }
        }
    }
    cout << "Total deposits for account " << accountNumber << ": "
         << fixed << setprecision(2) << totalDeposits << endl;
    return totalDeposits; // Return total deposits
}

// Generate unique transaction ID
string TransactionManager::generateTransactionId()
{
    string dateStr = getCurrentDate();
    dateStr.erase(remove(dateStr.begin(), dateStr.end(), '-'), dateStr.end());

    stringstream ss;
    ss << "TXN" << dateStr << setfill('0') << setw(3) << nextTransactionNumber++;
    return ss.str();
}

// Save transaction history to file
bool TransactionManager::saveTransactionHistory() const
{
    ofstream file(logFilePath);
    if (!file.is_open())
    {
        cout << "Failed to open transaction history file for writing: " << logFilePath << endl;
        return false; // File could not be opened
    }
    file << transactionHistory.size() << endl; // Write number of transactions
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        file << txn.getTransactionId() << " "
             << txn.getAccountNumber() << " "
             << txn.getRelatedAccountNumber() << " "
             << static_cast<int>(txn.getTransactionType()) << " "
             << fixed << setprecision(2) << txn.getAmount() << " "
             << fixed << setprecision(2) << txn.getBalanceBefore() << " "
             << fixed << setprecision(2) << txn.getBalanceAfter() << " "
             << txn.getTimestamp() << " "
             << txn.getDescription() << " "
             << static_cast<int>(txn.getStatus()) << " "
             << txn.getCustomerId() << " "
             << txn.getSessionId() << " "
             << txn.getNotes() << endl;
    }
    file.close();
    cout << "Transaction history saved successfully to: " << logFilePath << endl;
    return true;
}

// Load transaction history from file
bool TransactionManager::loadTransactionHistory()
{
    ifstream file(logFilePath);
    if (!file.is_open())
    {
        cout << "Transaction history file not found: " << logFilePath << endl;
        return false; // File does not exist
    }
    cout << "Loading transaction history from: " << logFilePath << endl;
    int transactionCount;
    file >> transactionCount;
    file.ignore(); // Ignore newline after count
    for (int i = 0; i < transactionCount; ++i)
    {
        Transaction transaction;
        string line;
        getline(file, line);
        istringstream iss(line);
        string txnId, accNum, relatedAccNum, typeStr, desc, timestamp,
            statusStr, custId, sessionId, notes;
        double amount, beforeBal, afterBal;
        int typeInt = 0, statusInt = 0;
        iss >> txnId >> accNum >> relatedAccNum >> typeInt >> amount >> beforeBal >> afterBal >> timestamp >> desc >> statusStr >> custId >> sessionId >> notes;
        transaction = Transaction(txnId, accNum, static_cast<TransactionType>(typeInt),
                                  amount, desc, beforeBal, afterBal, custId);
        transaction.setRelatedAccountNumber(relatedAccNum);
        transaction.setSessionId(sessionId);
        transaction.setNotes(notes);
        transaction.setStatus(static_cast<TransactionStatus>(statusInt));
        // Add to transaction history
        transactionHistory[txnId] = transaction;
        // Add to account transactions
        accountTransactions.emplace(accNum, txnId);
        // Add to customer transactions
        customerTransactions.emplace(custId, txnId);
        // Add to date transactions
        dateTransactions.emplace(timestamp.substr(0, 10), txnId); // Use date part
        // Update next transaction number
        size_t lastDash = txnId.find_last_of('-');
        if (lastDash != string::npos && lastDash + 1 < txnId.length())
        {
            string numericPart = txnId.substr(lastDash + 1);
            int txnNumber = stoi(numericPart);
            if (nextTransactionNumber <= txnNumber)
            {
                nextTransactionNumber = txnNumber + 1;
            }
        }
    }
    file.close();
    cout << "Loaded " << transactionCount << " transactions." << endl;
    return true; // Successfully loaded
}

// Display transaction statistics
void TransactionManager::displayTransactionStatistics() const
{
    // TODO: Display system-wide transaction statistics
    // Include: Total transactions, total volume, transaction types breakdown,
    // successful vs failed transactions, etc.
    cout << "Transaction Statistics:" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Transactions: " << transactionHistory.size() << endl;
    double totalVolume = 0.0;
    int successfulCount = 0;
    int failedCount = 0;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        totalVolume += txn.getNetAmount(); // Sum net amounts
        if (txn.getStatus() == TransactionStatus::COMPLETED)
        {
            successfulCount++;
        }
        else if (txn.getStatus() == TransactionStatus::FAILED)
        {
            failedCount++;
        }
    }
    cout << "Total Volume: " << fixed << setprecision(2) << totalVolume << endl;
    cout << "Successful Transactions: " << successfulCount << endl;
    cout << "Failed Transactions: " << failedCount << endl;
    cout << "----------------------------------------" << endl;
    cout << "Transaction Types Breakdown:" << endl;
    map<TransactionType, int> typeCount;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        typeCount[txn.getTransactionType()]++; // Count each type
    }
    for (const auto &pair : typeCount)
    {
        cout << "Type: " << static_cast<int>(pair.first)
             << ", Count: " << pair.second << endl;
    }
    cout << "----------------------------------------" << endl;
    cout << "End of Transaction Statistics" << endl;
    cout << "----------------------------------------" << endl;
}
// Process fee charge transaction
string TransactionManager::processFeeCharge(string accountNumber, double amount, string description,
                                            double balanceBefore, double balanceAfter, string customerId)
{
    string transactionId = generateTransactionId();
    Transaction transaction(transactionId, accountNumber, TransactionType::FEE_CHARGE, amount,
                            description, balanceBefore, balanceAfter, customerId);
    transaction.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (addTransaction(transaction))
    {
        cout << "Fee charge transaction processed successfully: " << transactionId << endl;
        return transactionId; // Return the transaction ID
    }
    else
    {
        cout << "Failed to process fee charge transaction." << endl;
        return ""; // Return empty string on failure
    }
}

// Process interest credit transaction
string TransactionManager::processInterestCredit(string accountNumber, double amount, string description,
                                                 double balanceBefore, double balanceAfter, string customerId)
{
    string transactionId = generateTransactionId();
    Transaction transaction(transactionId, accountNumber, TransactionType::INTEREST_CREDIT, amount,
                            description, balanceBefore, balanceAfter, customerId);
    transaction.setStatus(TransactionStatus::COMPLETED); // Set status to completed
    if (addTransaction(transaction))
    {
        cout << "Interest credit transaction processed successfully: " << transactionId << endl;
        return transactionId; // Return the transaction ID
    }
    else
    {
        cout << "Failed to process interest credit transaction." << endl;
        return ""; // Return empty string on failure
    }
}

// Get transactions by date range
vector<Transaction> TransactionManager::getTransactionsByDateRange(string startDate, string endDate) const
{
    vector<Transaction> transactions;
    for (const auto &pair : dateTransactions)
    {
        const string &date = pair.first;
        const string &transactionId = pair.second;
        if (date >= startDate && date <= endDate)
        {
            auto it = transactionHistory.find(transactionId);
            if (it != transactionHistory.end())
            {
                transactions.push_back(it->second); // Add transaction to vector
            }
        }
    }
    // Sort transactions by timestamp (newest first)
    sort(transactions.begin(), transactions.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() > b.getTimestamp(); // Newest first
         });
    cout << "Retrieved " << transactions.size() << " transactions from "
         << startDate << " to " << endDate << "." << endl;
    return transactions; // Return the filtered transactions
}

// Get transactions by amount range
vector<Transaction> TransactionManager::getTransactionsByAmountRange(double minAmount, double maxAmount) const
{
    vector<Transaction> transactions;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        double netAmount = txn.getNetAmount();
        if (netAmount >= minAmount && netAmount <= maxAmount)
        {
            transactions.push_back(txn); // Add to filtered transactions
        }
    }
    // Sort transactions by amount (ascending)
    sort(transactions.begin(), transactions.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getNetAmount() < b.getNetAmount(); // Ascending order
         });
    cout << "Retrieved " << transactions.size() << " transactions in amount range ["
         << minAmount << ", " << maxAmount << "]." << endl;
    return transactions; // Return the filtered transactions
}

// Get failed transactions
vector<Transaction> TransactionManager::getFailedTransactions() const
{
    vector<Transaction> failedTransactions;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        if (txn.getStatus() == TransactionStatus::FAILED)
        {
            failedTransactions.push_back(txn); // Add to failed transactions
        }
    }
    // Sort failed transactions by timestamp (newest first)
    sort(failedTransactions.begin(), failedTransactions.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() > b.getTimestamp(); // Newest first
         });
    cout << "Retrieved " << failedTransactions.size() << " failed transactions." << endl;
    return failedTransactions; // Return the vector of failed transactions
}

// Get pending transactions
vector<Transaction> TransactionManager::getPendingTransactions() const
{
    vector<Transaction> pendingTransactions;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        if (txn.getStatus() == TransactionStatus::PENDING)
        {
            pendingTransactions.push_back(txn); // Add to pending transactions
        }
    }
    // Sort pending transactions by timestamp (oldest first)
    sort(pendingTransactions.begin(), pendingTransactions.end(), [](const Transaction &a, const Transaction &b)
         {
             return a.getTimestamp() < b.getTimestamp(); // Oldest first
         });
    cout << "Retrieved " << pendingTransactions.size() << " pending transactions." << endl;
    return pendingTransactions; // Return the vector of pending transactions
}

// Get total withdrawals for account
double TransactionManager::getTotalWithdrawals(string accountNumber, string startDate, string endDate) const
{
    double totalWithdrawals = 0.0;
    vector<Transaction> history = getAccountHistory(accountNumber, INT_MAX); // Get all transactions
    for (const Transaction &txn : history)
    {
        if (txn.getTransactionType() == TransactionType::WITHDRAWAL ||
            (txn.getTransactionType() == TransactionType::TRANSFER_OUT))
        {
            if (startDate.empty() || endDate.empty() ||
                (txn.getTimestamp() >= startDate && txn.getTimestamp() <= endDate))
            {
                totalWithdrawals += txn.getAmount(); // Add to total withdrawals
            }
        }
    }
    cout << "Total withdrawals for account " << accountNumber << ": "
         << fixed << setprecision(2) << totalWithdrawals << endl;
    return totalWithdrawals; // Return total withdrawals
}

// Get net flow for account
double TransactionManager::getNetFlow(string accountNumber, string startDate, string endDate) const
{
    double totalDeposits = getTotalDeposits(accountNumber, startDate, endDate);
    double totalWithdrawals = getTotalWithdrawals(accountNumber, startDate, endDate);
    double netFlow = totalDeposits - totalWithdrawals;
    cout << "Net flow for account " << accountNumber << ": "
         << fixed << setprecision(2) << netFlow << endl;
    return netFlow; // Return net flow
}

// Get transaction count for account
int TransactionManager::getTransactionCount(string accountNumber, string startDate, string endDate) const
{
    vector<Transaction> history = getAccountHistory(accountNumber, INT_MAX); // Get all transactions
    int count = 0;
    for (const Transaction &txn : history)
    {
        if (startDate.empty() || endDate.empty() ||
            (txn.getTimestamp() >= startDate && txn.getTimestamp() <= endDate))
        {
            count++; // Increment count for each transaction in date range
        }
    }
    cout << "Transaction count for account " << accountNumber << ": "
         << count << endl;
    return count; // Return transaction count
}

// Get average transaction amount by type
double TransactionManager::getAverageTransactionAmount(string accountNumber, TransactionType type) const
{
    vector<Transaction> history = getAccountHistory(accountNumber, INT_MAX); // Get all transactions
    double totalAmount = 0.0;
    int count = 0;
    for (const Transaction &txn : history)
    {
        if (txn.getTransactionType() == type)
        {
            totalAmount += txn.getAmount(); // Add to total amount
            count++;                        // Increment count
        }
    }
    double average = (count > 0) ? (totalAmount / count) : 0.0; // Calculate average
    cout << "Average transaction amount for account " << accountNumber
         << " of type " << static_cast<int>(type) << ": "
         << fixed << setprecision(2) << average << endl;
    return average; // Return average amount
}

// Get current date and time
string TransactionManager::getCurrentDateTime() const
{
    return ::getCurrentDateTime(); // Use the global function from Utilities
}

// Display customer transaction summary
void TransactionManager::displayCustomerTransactionSummary(string customerId) const
{
    cout << "\n=== Transaction Summary for Customer: " << customerId << " ===" << endl;

    vector<Transaction> customerTxns = getCustomerHistory(customerId, 100);
    if (customerTxns.empty())
    {
        cout << "No transactions found for this customer." << endl;
        return;
    }

    // Calculate statistics
    double totalDeposits = 0, totalWithdrawals = 0, totalTransfers = 0, totalFees = 0, totalInterest = 0;
    int depositCount = 0, withdrawalCount = 0, transferCount = 0, feeCount = 0, interestCount = 0, otherCount = 0;

    for (const auto &txn : customerTxns)
    {
        switch (txn.getTransactionType())
        {
        case TransactionType::DEPOSIT:
            totalDeposits += txn.getAmount();
            depositCount++;
            break;
        case TransactionType::WITHDRAWAL:
            totalWithdrawals += txn.getAmount();
            withdrawalCount++;
            break;
        case TransactionType::TRANSFER_IN:
        case TransactionType::TRANSFER_OUT:
            totalTransfers += txn.getAmount();
            transferCount++;
            break;
        case TransactionType::FEE_CHARGE:
            totalFees += txn.getAmount();
            feeCount++;
            break;
        case TransactionType::INTEREST_CREDIT:
            totalInterest += txn.getAmount();
            interestCount++;
            break;
        case TransactionType::ACCOUNT_OPENING:
        case TransactionType::ACCOUNT_CLOSING:
        default:
            otherCount++;
            break;
        }
    }

    // Update the display section to include new categories:
    cout << "Total Transactions: " << customerTxns.size() << endl;
    cout << "Deposits: " << depositCount << " ($" << fixed << setprecision(2) << totalDeposits << ")" << endl;
    cout << "Withdrawals: " << withdrawalCount << " ($" << fixed << setprecision(2) << totalWithdrawals << ")" << endl;
    cout << "Transfers: " << transferCount << " ($" << fixed << setprecision(2) << totalTransfers << ")" << endl;
    cout << "Fees: " << feeCount << " ($" << fixed << setprecision(2) << totalFees << ")" << endl;
    cout << "Interest: " << interestCount << " ($" << fixed << setprecision(2) << totalInterest << ")" << endl;
    cout << "Other: " << otherCount << endl;
    cout << "Net Flow: $" << fixed << setprecision(2) << (totalDeposits + totalInterest - totalWithdrawals - totalFees) << endl;

    // Display recent transactions
    cout << "\nRecent Transactions (Last 5):" << endl;
    cout << left << setw(15) << "Transaction ID" << setw(12) << "Type" << setw(10) << "Amount" << setw(12) << "Date" << endl;
    cout << string(50, '-') << endl;

    int count = 0;
    for (const auto &txn : customerTxns)
    {
        if (count >= 5)
            break;
        cout << left << setw(15) << txn.getTransactionId()
             << setw(12) << txn.getTransactionTypeString()
             << setw(10) << fixed << setprecision(2) << txn.getAmount()
             << setw(12) << txn.getTimestamp().substr(0, 10) << endl;
        count++;
    }
    cout << "======================================================" << endl;
}
// Get total transaction count
int TransactionManager::getTotalTransactionCount() const
{
    return transactionHistory.size();
}

// Get total system volume
double TransactionManager::getTotalSystemVolume() const
{
    double totalVolume = 0.0;
    for (const auto &txn : transactionHistory)
    {
        totalVolume += txn.second.getNetAmount(); // Sum net amounts of all transactions
    }
    return totalVolume;
}
// Display transactions by type
void TransactionManager::displayTransactionsByType(TransactionType type, int limit) const
{
    vector<Transaction> transactions = getTransactionsByType(type, limit);
    cout << "Transactions of Type: " << static_cast<int>(type) << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << left << setw(15) << "Transaction ID"
         << setw(20) << "Account Number"
         << setw(10) << "Amount"
         << setw(20) << "Date"
         << setw(20) << "Status"
         << setw(30) << "Description" << endl;
    cout << "--------------------------------------------------------" << endl;
    double totalAmount = 0.0;
    for (const Transaction &txn : transactions)
    {
        cout << left << setw(15) << txn.getTransactionId()
             << setw(20) << txn.getAccountNumber()
             << setw(10) << fixed << setprecision(2) << txn.getAmount()
             << setw(20) << txn.getTimestamp().substr(0, 10)
             << setw(20) << txn.getStatusString()
             << setw(30) << txn.getDescription() << endl;
        totalAmount += txn.getAmount(); // Accumulate total amount
    }
    cout << "--------------------------------------------------------" << endl;
    cout << left << setw(15) << "Total"
         << setw(20) << ""
         << setw(10) << fixed << setprecision(2) << totalAmount
         << setw(20) << ""
         << setw(20) << ""
         << setw(30) << "" << endl;
    cout << "Total Transactions: " << transactions.size() << endl;
    cout << "Total Amount: " << fixed << setprecision(2) << totalAmount << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << "End of Transactions of Type: " << static_cast<int>(type) << endl;
    cout << "--------------------------------------------------------" << endl;
}

// Display daily transaction summary
void TransactionManager::displayDailyTransactionSummary(string date) const
{
    vector<Transaction> transactions = getTransactionsByDateRange(date, date);
    if (transactions.empty())
    {
        cout << "No transactions found for date: " << date << endl;
        return; // No transactions for the date
    }

    // Group transactions by type
    map<TransactionType, vector<Transaction>> groupedTransactions;
    for (const auto &txn : transactions)
    {
        groupedTransactions[txn.getTransactionType()].push_back(txn);
    }

    // Calculate totals for each type
    map<TransactionType, double> typeTotals;
    for (const auto &[type, txns] : groupedTransactions)
    {
        double total = 0.0;
        for (const auto &txn : txns)
        {
            total += txn.getAmount();
        }
        typeTotals[type] = total;
    }

    // Display report
    cout << "Daily Transaction Summary for Date: " << date << endl;
    cout << "--------------------------------------------------------" << endl;
    for (const auto &[type, total] : typeTotals)
    {
        cout << "Transaction Type: " << static_cast<int>(type) << endl;
        cout << "Total Transactions: " << groupedTransactions[type].size() << endl;
        cout << "Total Amount: " << fixed << setprecision(2) << total << endl;
        cout << "--------------------------------------------------------" << endl;
    }

    // Display all transactions
    cout << "All Transactions for Date: " << date << endl;
    cout << "--------------------------------------------------------" << endl;
    for (const auto &txn : transactions)
    {
        cout << left << setw(15) << txn.getTransactionId()
             << setw(20) << txn.getAccountNumber()
             << setw(10) << fixed << setprecision(2) << txn.getAmount()
             << setw(20) << txn.getTimestamp().substr(0, 10)
             << setw(20) << txn.getStatusString()
             << setw(30) << txn.getDescription() << endl;
    }
    cout << "--------------------------------------------------------" << endl;
    double netFlow = 0.0;
    for (const auto &txn : transactions)
    {
        netFlow += txn.getNetAmount(); // Calculate net flow for the day
    }
    cout << "Net Flow for Date: " << date << " is " << fixed << setprecision(2) << netFlow << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << "End of Daily Transaction Summary" << endl;
    cout << "--------------------------------------------------------" << endl;
}
// Validate transaction ID format
bool TransactionManager::isValidTransactionId(string transactionId) const
{
    if (transactionId.size() < 15 || transactionId.substr(0, 3) != "TXN")
    {
        return false; // Invalid prefix
    }
    string datePart = transactionId.substr(3, 10); // Extract date part
    if (datePart.size() != 10 || datePart[4] != '-' || datePart[7] != '-')
    {
        return false; // Invalid date format
    }
    string seqPart = transactionId.substr(13); // Extract sequence part
    if (seqPart.empty() || !isdigit(seqPart[0]))
    {
        return false; // Invalid sequence format
    }
    return true; // Valid format
}

// Get current date (wrapper function)
string TransactionManager::getCurrentDate() const
{
    return ::getCurrentDateTime().substr(0, 10); // Return current date in YYYY-MM-DD format
}
// Export transactions to CSV
bool TransactionManager::exportTransactionsToCSV(string filename, string accountNumber) const
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Failed to open file for writing: " << filename << endl;
        return false; // File could not be opened
    }

    // Write CSV header
    file << "ID,Account,Type,Amount,Date,Status,Description" << endl;

    // Write each transaction as CSV row
    for (const auto &txn : transactionHistory)
    {
        if (!accountNumber.empty() && txn.second.getAccountNumber() != accountNumber)
        {
            continue; // Skip transactions not matching accountNumber
        }
        file << txn.second.getTransactionId() << ","
             << txn.second.getAccountNumber() << ","
             << static_cast<int>(txn.second.getTransactionType()) << ","
             << fixed << setprecision(2) << txn.second.getAmount() << ","
             << txn.second.getTimestamp().substr(0, 10) << ","
             << txn.second.getStatusString() << ","
             << txn.second.getDescription() << endl;
    }

    file.close();
    return true; // Export successful
}

// Create backup of transaction data
bool TransactionManager::createBackup(string backupPath) const
{
    string backupFile = backupPath + "/transaction_backup_" + getCurrentDateTime() + ".log";
    ifstream src(logFilePath, ios::binary);
    ofstream dst(backupFile, ios::binary);
    if (!src.is_open() || !dst.is_open())
    {
        cout << "Failed to create backup file: " << backupFile << endl;
        return false; // Backup file could not be opened
    }
    dst << src.rdbuf(); // Copy contents
    src.close();
    dst.close();
    cout << "Backup created successfully at: " << backupFile << endl;
    return true; // Backup successful
}

// Cleanup old transactions (archival)
void TransactionManager::cleanupOldTransactions(int daysOld)
{
    string cutoffDate = getCurrentDateTime(); // Placeholder for actual cutoff date calculation
    cout << "Cleaning up transactions older than " << daysOld << " days..." << endl;
    int removedCount = 0;
    for (auto it = transactionHistory.begin(); it != transactionHistory.end();)
    {
        const Transaction &txn = it->second;
        if (txn.getTimestamp() < cutoffDate) // Check if transaction is older than cutoff
        {
            // Remove from all indexes
            accountTransactions.erase(txn.getAccountNumber());
            customerTransactions.erase(txn.getCustomerId());
            dateTransactions.erase(txn.getTimestamp().substr(0, 10)); // Use date part only
            it = transactionHistory.erase(it);                        // Remove from history and get next iterator
            removedCount++;                                           // Increment removed count
            cout << "Removed transaction: " << txn.getTransactionId() << endl;
        }
        else
        {
            ++it; // Move to next transaction
        }
    }
    cout << "Cleanup complete. Removed " << removedCount << " old transactions." << endl;
}
// Rebuild transaction indexes
void TransactionManager::reindexTransactions()
{
    cout << "Rebuilding transaction indexes..." << endl;
    accountTransactions.clear();
    customerTransactions.clear();
    dateTransactions.clear();
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        accountTransactions.emplace(txn.getAccountNumber(), txn.getTransactionId());
        customerTransactions.emplace(txn.getCustomerId(), txn.getTransactionId());
        dateTransactions.emplace(txn.getTimestamp().substr(0, 10), txn.getTransactionId()); // Use date part only
    }
    cout << "Transaction indexes rebuilt successfully." << endl;
    cout << "Total Transactions: " << transactionHistory.size() << endl;
    cout << "Total Accounts: " << accountTransactions.size() << endl;
    cout << "Total Customers: " << customerTransactions.size() << endl;
    cout << "Total Dates: " << dateTransactions.size() << endl;
    cout << "Reindexing complete." << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << "End of Transaction Manager Operations" << endl;
    cout << "--------------------------------------------------------" << endl;
}
