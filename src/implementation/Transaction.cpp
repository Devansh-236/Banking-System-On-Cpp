#include "Transaction.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// =============================================================================
// HELPERS
// =============================================================================

// Numeric sequence part of a TXNyyyymmddNNNNNN id (-1 if not in that format)
static long long transactionSequence(const string& transactionId) {
    if (transactionId.size() < 12 || transactionId.compare(0, 3, "TXN") != 0) {
        return -1;
    }
    string digits = transactionId.substr(11);
    if (digits.empty() || !all_of(digits.begin(), digits.end(), [](char c) { return isdigit(static_cast<unsigned char>(c)); })) {
        return -1;
    }
    try {
        return stoll(digits);
    } catch (...) {
        return -1;
    }
}

// Newest first: by timestamp, then by sequence number for same-second transactions
static bool newestFirst(const Transaction& a, const Transaction& b) {
    if (a.getTimestamp() != b.getTimestamp()) {
        return a.getTimestamp() > b.getTimestamp();
    }
    return transactionSequence(a.getTransactionId()) > transactionSequence(b.getTransactionId());
}

// Quote a CSV field if it contains separators or quotes
static string csvField(const string& value) {
    if (value.find_first_of(",\"\n") == string::npos) {
        return value;
    }
    string quoted = "\"";
    for (char c : value) {
        if (c == '"') quoted += "\"\"";
        else quoted += c;
    }
    return quoted + "\"";
}

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
    timestamp = ::getCurrentDateTime();
    description = "";
    balanceBefore = 0.0;
    balanceAfter = 0.0;
    status = TransactionStatus::PENDING;
    customerId = "";
    sessionId = "";
    notes = "";
    linkedTransactionId = "";
}

// Parameterized constructor
Transaction::Transaction(string txnId, string accNum, TransactionType type, double amt,
                         string desc, double beforeBal, double afterBal, string custId)
{
    transactionId = txnId;
    accountNumber = accNum;
    relatedAccountNumber = "";
    transactionType = type;
    amount = roundMoney(amt);
    timestamp = ::getCurrentDateTime();
    description = trim(desc);
    balanceBefore = roundMoney(beforeBal);
    balanceAfter = roundMoney(afterBal);
    status = TransactionStatus::COMPLETED; // Default to completed
    customerId = custId;
    sessionId = "";
    notes = "";
    linkedTransactionId = "";
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

// Get date part of the timestamp
string Transaction::getDate() const
{
    return timestamp.substr(0, 10);
}

// Get description
string Transaction::getDescription() const
{
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
    return customerId;
}

// Get session ID
string Transaction::getSessionId() const
{
    return sessionId;
}

// Get notes
string Transaction::getNotes() const
{
    return notes;
}

// Get linked transaction ID
string Transaction::getLinkedTransactionId() const
{
    return linkedTransactionId;
}

// Set transaction status
void Transaction::setStatus(TransactionStatus newStatus)
{
    status = newStatus;
}

// Set related account number
void Transaction::setRelatedAccountNumber(string relatedAccNum)
{
    relatedAccountNumber = (relatedAccNum == "N/A") ? "" : relatedAccNum;
}

// Set session ID
void Transaction::setSessionId(string sessionId)
{
    this->sessionId = sessionId;
}

// Set notes
void Transaction::setNotes(string notes)
{
    this->notes = notes;
}

// Set balance after transaction (may be negative for overdrawn checking accounts)
void Transaction::setBalanceAfter(double balance)
{
    balanceAfter = roundMoney(balance);
}

// Set timestamp (used when loading saved transactions)
void Transaction::setTimestamp(string newTimestamp)
{
    if (newTimestamp.size() >= 10) {
        timestamp = newTimestamp;
    }
}

// Set linked transaction ID
void Transaction::setLinkedTransactionId(string linkedId)
{
    linkedTransactionId = linkedId;
}

string Transaction::typeToString(TransactionType type)
{
    switch (type)
    {
    case TransactionType::DEPOSIT:           return "DEPOSIT";
    case TransactionType::WITHDRAWAL:        return "WITHDRAWAL";
    case TransactionType::TRANSFER_IN:       return "TRANSFER_IN";
    case TransactionType::TRANSFER_OUT:      return "TRANSFER_OUT";
    case TransactionType::FEE_CHARGE:        return "FEE_CHARGE";
    case TransactionType::INTEREST_CREDIT:   return "INTEREST_CREDIT";
    case TransactionType::ACCOUNT_OPENING:   return "ACCOUNT_OPENING";
    case TransactionType::ACCOUNT_CLOSING:   return "ACCOUNT_CLOSING";
    case TransactionType::LOAN_DISBURSEMENT: return "LOAN_DISBURSEMENT";
    case TransactionType::LOAN_PAYMENT:      return "LOAN_PAYMENT";
    default:                                 return "UNKNOWN";
    }
}

TransactionType Transaction::typeFromString(const string& name)
{
    static const TransactionType all[] = {
        TransactionType::DEPOSIT, TransactionType::WITHDRAWAL, TransactionType::TRANSFER_IN,
        TransactionType::TRANSFER_OUT, TransactionType::FEE_CHARGE, TransactionType::INTEREST_CREDIT,
        TransactionType::ACCOUNT_OPENING, TransactionType::ACCOUNT_CLOSING,
        TransactionType::LOAN_DISBURSEMENT, TransactionType::LOAN_PAYMENT};
    string upper = toUpper(trim(name));
    for (TransactionType type : all) {
        if (typeToString(type) == upper) return type;
    }
    return TransactionType::UNKNOWN;
}

string Transaction::statusToString(TransactionStatus status)
{
    switch (status)
    {
    case TransactionStatus::PENDING:   return "PENDING";
    case TransactionStatus::COMPLETED: return "COMPLETED";
    case TransactionStatus::FAILED:    return "FAILED";
    case TransactionStatus::CANCELLED: return "CANCELLED";
    case TransactionStatus::REVERSED:  return "REVERSED";
    default:                           return "UNKNOWN";
    }
}

TransactionStatus Transaction::statusFromString(const string& name)
{
    string upper = toUpper(trim(name));
    if (upper == "COMPLETED") return TransactionStatus::COMPLETED;
    if (upper == "FAILED") return TransactionStatus::FAILED;
    if (upper == "CANCELLED") return TransactionStatus::CANCELLED;
    if (upper == "REVERSED") return TransactionStatus::REVERSED;
    return TransactionStatus::PENDING;
}

// Get transaction type as string
string Transaction::getTransactionTypeString() const
{
    return typeToString(transactionType);
}

// Get status as string
string Transaction::getStatusString() const
{
    return statusToString(status);
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

// Check if the transaction puts money into the account
bool Transaction::isCredit() const
{
    switch (transactionType)
    {
    case TransactionType::DEPOSIT:
    case TransactionType::TRANSFER_IN:
    case TransactionType::INTEREST_CREDIT:
    case TransactionType::ACCOUNT_OPENING:
    case TransactionType::LOAN_DISBURSEMENT:
        return true;
    default:
        return false;
    }
}

// Completed transactions moved money; reversed ones did too, and are offset by a compensating entry
bool Transaction::affectsBalance() const
{
    return status == TransactionStatus::COMPLETED || status == TransactionStatus::REVERSED;
}

// Get net amount (positive for credits, negative for debits)
double Transaction::getNetAmount() const
{
    if (transactionType == TransactionType::UNKNOWN) {
        return 0.0;
    }
    return isCredit() ? amount : -amount;
}

// Display detailed transaction information
void Transaction::displayTransaction() const
{
    cout << "Transaction ID   : " << transactionId << endl;
    cout << "Account Number   : " << accountNumber << endl;
    cout << "Related Account  : " << (relatedAccountNumber.empty() ? "N/A" : relatedAccountNumber) << endl;
    cout << "Type             : " << getTransactionTypeString() << endl;
    cout << "Amount           : " << formatCurrency(amount) << endl;
    cout << "Timestamp        : " << timestamp << endl;
    cout << "Description      : " << (description.empty() ? "-" : description) << endl;
    cout << "Balance Before   : " << formatCurrency(balanceBefore) << endl;
    cout << "Balance After    : " << formatCurrency(balanceAfter) << endl;
    cout << "Status           : " << getStatusString() << endl;
    cout << "Customer ID      : " << (customerId.empty() ? "N/A" : customerId) << endl;
    cout << "Session ID       : " << (sessionId.empty() ? "N/A" : sessionId) << endl;
    cout << "Linked Txn       : " << (linkedTransactionId.empty() ? "N/A" : linkedTransactionId) << endl;
    cout << "Notes            : " << (notes.empty() ? "-" : notes) << endl;
}

// Display transaction summary (one line)
void Transaction::displayTransactionSummary() const
{
    cout << transactionId
         << " | " << timestamp
         << " | " << left << setw(17) << getTransactionTypeString()
         << " | " << right << setw(12) << formatCurrency(getNetAmount())
         << " | " << getStatusString()
         << " | " << description
         << endl;
}

// Comparison operator for sorting (chronological)
bool Transaction::operator<(const Transaction &other) const
{
    if (timestamp != other.timestamp) {
        return timestamp < other.timestamp;
    }
    return transactionSequence(transactionId) < transactionSequence(other.transactionId);
}

// Equality operator
bool Transaction::operator==(const Transaction &other) const
{
    return transactionId == other.transactionId;
}

// =============================================================================
// TRANSACTION MANAGER CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor (data is loaded explicitly via loadTransactionHistory)
TransactionManager::TransactionManager(string logFile)
{
    logFilePath = logFile;
    nextTransactionNumber = 1;
    currentSessionId = "";
}

void TransactionManager::setSessionContext(string sessionId)
{
    currentSessionId = sessionId;
}

string TransactionManager::getSessionContext() const
{
    return currentSessionId;
}

// Add a transaction to every lookup index
void TransactionManager::indexTransaction(const Transaction &transaction)
{
    const string &transactionId = transaction.getTransactionId();
    accountTransactions.emplace(transaction.getAccountNumber(), transactionId);
    if (!transaction.getCustomerId().empty()) {
        customerTransactions.emplace(transaction.getCustomerId(), transactionId);
    }
    dateTransactions.emplace(transaction.getDate(), transactionId);
}

// Keep the ID counter ahead of every ID we have seen
void TransactionManager::updateNextTransactionNumber(const string &transactionId)
{
    long long sequence = transactionSequence(transactionId);
    if (sequence >= nextTransactionNumber) {
        nextTransactionNumber = sequence + 1;
    }
}

// Inclusive date filter on the YYYY-MM-DD part; empty bounds are open
bool TransactionManager::inDateRange(const Transaction &txn, const string &startDate, const string &endDate)
{
    string date = txn.getDate();
    if (!startDate.empty() && date < startDate) return false;
    if (!endDate.empty() && date > endDate) return false;
    return true;
}

// Generic transaction recorder used by every process* function
string TransactionManager::processTransaction(TransactionType type, string accountNumber, double amount,
                                              string description, double balanceBefore, double balanceAfter,
                                              string customerId, string relatedAccount)
{
    string transactionId = generateTransactionId();
    Transaction transaction(transactionId, accountNumber, type, amount,
                            description, balanceBefore, balanceAfter, customerId);
    transaction.setRelatedAccountNumber(relatedAccount);
    transaction.setSessionId(currentSessionId);
    transaction.setStatus(TransactionStatus::COMPLETED);
    return addTransaction(transaction) ? transactionId : "";
}

// Process deposit transaction
string TransactionManager::processDeposit(string accountNumber, double amount, string description,
                                          double balanceBefore, double balanceAfter, string customerId)
{
    return processTransaction(TransactionType::DEPOSIT, accountNumber, amount, description,
                              balanceBefore, balanceAfter, customerId);
}

// Process withdrawal transaction
string TransactionManager::processWithdrawal(string accountNumber, double amount, string description,
                                             double balanceBefore, double balanceAfter, string customerId)
{
    return processTransaction(TransactionType::WITHDRAWAL, accountNumber, amount, description,
                              balanceBefore, balanceAfter, customerId);
}

// Process transfer: records a TRANSFER_OUT and a linked TRANSFER_IN; returns the OUT id
string TransactionManager::processTransfer(string fromAccount, string toAccount, double amount,
                                           string description, double fromBalBefore, double fromBalAfter,
                                           double toBalBefore, double toBalAfter, string fromCustomerId,
                                           string toCustomerId)
{
    if (toCustomerId.empty()) {
        toCustomerId = fromCustomerId;
    }
    string transferOutId = processTransaction(TransactionType::TRANSFER_OUT, fromAccount, amount, description,
                                              fromBalBefore, fromBalAfter, fromCustomerId, toAccount);
    if (transferOutId.empty()) {
        return "";
    }
    string transferInId = processTransaction(TransactionType::TRANSFER_IN, toAccount, amount, description,
                                             toBalBefore, toBalAfter, toCustomerId, fromAccount);
    if (transferInId.empty()) {
        updateTransactionStatus(transferOutId, TransactionStatus::FAILED);
        return "";
    }
    linkTransactions(transferOutId, transferInId);
    return transferOutId;
}

// Process fee charge transaction
string TransactionManager::processFeeCharge(string accountNumber, double amount, string description,
                                            double balanceBefore, double balanceAfter, string customerId)
{
    return processTransaction(TransactionType::FEE_CHARGE, accountNumber, amount, description,
                              balanceBefore, balanceAfter, customerId);
}

// Process interest credit transaction
string TransactionManager::processInterestCredit(string accountNumber, double amount, string description,
                                                 double balanceBefore, double balanceAfter, string customerId)
{
    return processTransaction(TransactionType::INTEREST_CREDIT, accountNumber, amount, description,
                              balanceBefore, balanceAfter, customerId);
}

// Add transaction to system
bool TransactionManager::addTransaction(Transaction transaction)
{
    const string transactionId = transaction.getTransactionId();
    if (transactionId.empty() || transactionHistory.count(transactionId) > 0)
    {
        return false; // Missing or duplicate ID
    }
    transactionHistory[transactionId] = transaction;
    indexTransaction(transaction);
    updateNextTransactionNumber(transactionId);
    return true;
}

// Find transaction by ID
Transaction *TransactionManager::findTransaction(string transactionId)
{
    auto it = transactionHistory.find(transactionId);
    return it != transactionHistory.end() ? &it->second : nullptr;
}

const Transaction *TransactionManager::findTransaction(string transactionId) const
{
    auto it = transactionHistory.find(transactionId);
    return it != transactionHistory.end() ? &it->second : nullptr;
}

// Update transaction status (indexes do not depend on status, so nothing else changes)
bool TransactionManager::updateTransactionStatus(string transactionId, TransactionStatus newStatus)
{
    Transaction *transaction = findTransaction(transactionId);
    if (!transaction)
    {
        return false;
    }
    transaction->setStatus(newStatus);
    return true;
}

// Mark a completed transaction as reversed. The Bank creates the compensating
// entry and adjusts balances, because only it can see the accounts.
bool TransactionManager::reverseTransaction(string transactionId, string reason)
{
    Transaction *transaction = findTransaction(transactionId);
    if (!transaction || transaction->getStatus() != TransactionStatus::COMPLETED)
    {
        return false;
    }
    transaction->setStatus(TransactionStatus::REVERSED);
    transaction->setNotes("Reversed: " + reason);
    return true;
}

// Link two transactions to each other (transfer legs, reversal pairs)
bool TransactionManager::linkTransactions(string firstId, string secondId)
{
    Transaction *first = findTransaction(firstId);
    Transaction *second = findTransaction(secondId);
    if (!first || !second)
    {
        return false;
    }
    first->setLinkedTransactionId(secondId);
    second->setLinkedTransactionId(firstId);
    return true;
}

// Get every transaction (newest first)
vector<Transaction> TransactionManager::getAllTransactions() const
{
    vector<Transaction> all;
    all.reserve(transactionHistory.size());
    for (const auto &pair : transactionHistory)
    {
        all.push_back(pair.second);
    }
    sort(all.begin(), all.end(), newestFirst);
    return all;
}

// Get account transaction history (newest first, at most limit entries)
vector<Transaction> TransactionManager::getAccountHistory(string accountNumber, int limit) const
{
    vector<Transaction> history;
    auto range = accountTransactions.equal_range(accountNumber);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto txnIt = transactionHistory.find(it->second);
        if (txnIt != transactionHistory.end())
        {
            history.push_back(txnIt->second);
        }
    }
    sort(history.begin(), history.end(), newestFirst);
    if (limit >= 0 && history.size() > static_cast<size_t>(limit))
    {
        history.resize(limit);
    }
    return history;
}

// Get customer transaction history (newest first, at most limit entries)
vector<Transaction> TransactionManager::getCustomerHistory(string customerId, int limit) const
{
    vector<Transaction> history;
    auto range = customerTransactions.equal_range(customerId);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto txnIt = transactionHistory.find(it->second);
        if (txnIt != transactionHistory.end())
        {
            history.push_back(txnIt->second);
        }
    }
    sort(history.begin(), history.end(), newestFirst);
    if (limit >= 0 && history.size() > static_cast<size_t>(limit))
    {
        history.resize(limit);
    }
    return history;
}

// Get transactions by type (newest first)
vector<Transaction> TransactionManager::getTransactionsByType(TransactionType type, int limit) const
{
    vector<Transaction> transactions;
    for (const auto &pair : transactionHistory)
    {
        if (pair.second.getTransactionType() == type)
        {
            transactions.push_back(pair.second);
        }
    }
    sort(transactions.begin(), transactions.end(), newestFirst);
    if (limit >= 0 && transactions.size() > static_cast<size_t>(limit))
    {
        transactions.resize(limit);
    }
    return transactions;
}

// Get transactions by date range (inclusive, newest first)
vector<Transaction> TransactionManager::getTransactionsByDateRange(string startDate, string endDate) const
{
    vector<Transaction> transactions;
    auto begin = startDate.empty() ? dateTransactions.begin() : dateTransactions.lower_bound(startDate);
    auto end = endDate.empty() ? dateTransactions.end() : dateTransactions.upper_bound(endDate);
    for (auto it = begin; it != end; ++it)
    {
        auto txnIt = transactionHistory.find(it->second);
        if (txnIt != transactionHistory.end())
        {
            transactions.push_back(txnIt->second);
        }
    }
    sort(transactions.begin(), transactions.end(), newestFirst);
    return transactions;
}

// Get one account's transactions in a date range (inclusive, newest first)
vector<Transaction> TransactionManager::getAccountTransactionsByDateRange(string accountNumber, string startDate,
                                                                          string endDate) const
{
    vector<Transaction> transactions;
    for (const Transaction &txn : getAccountHistory(accountNumber, -1))
    {
        if (inDateRange(txn, startDate, endDate))
        {
            transactions.push_back(txn);
        }
    }
    return transactions;
}

// Get transactions by amount range (ascending by amount)
vector<Transaction> TransactionManager::getTransactionsByAmountRange(double minAmount, double maxAmount) const
{
    vector<Transaction> transactions;
    for (const auto &pair : transactionHistory)
    {
        double amount = pair.second.getAmount();
        if (amount >= minAmount && amount <= maxAmount)
        {
            transactions.push_back(pair.second);
        }
    }
    sort(transactions.begin(), transactions.end(), [](const Transaction &a, const Transaction &b)
         { return a.getAmount() < b.getAmount(); });
    return transactions;
}

// Get failed transactions (newest first)
vector<Transaction> TransactionManager::getFailedTransactions() const
{
    vector<Transaction> failedTransactions;
    for (const auto &pair : transactionHistory)
    {
        if (pair.second.getStatus() == TransactionStatus::FAILED)
        {
            failedTransactions.push_back(pair.second);
        }
    }
    sort(failedTransactions.begin(), failedTransactions.end(), newestFirst);
    return failedTransactions;
}

// Get pending transactions (oldest first)
vector<Transaction> TransactionManager::getPendingTransactions() const
{
    vector<Transaction> pendingTransactions;
    for (const auto &pair : transactionHistory)
    {
        if (pair.second.getStatus() == TransactionStatus::PENDING)
        {
            pendingTransactions.push_back(pair.second);
        }
    }
    sort(pendingTransactions.begin(), pendingTransactions.end());
    return pendingTransactions;
}

// Get transactions at or above a threshold amount (largest first)
vector<Transaction> TransactionManager::getLargeTransactions(double threshold) const
{
    vector<Transaction> largeTransactions;
    for (const auto &pair : transactionHistory)
    {
        if (pair.second.getAmount() >= threshold)
        {
            largeTransactions.push_back(pair.second);
        }
    }
    sort(largeTransactions.begin(), largeTransactions.end(), [](const Transaction &a, const Transaction &b)
         { return a.getAmount() > b.getAmount(); });
    return largeTransactions;
}

// Most recent balance-affecting transaction for an account (nullptr if none)
const Transaction *TransactionManager::getLatestAccountTransaction(string accountNumber) const
{
    const Transaction *latest = nullptr;
    auto range = accountTransactions.equal_range(accountNumber);
    for (auto it = range.first; it != range.second; ++it)
    {
        const Transaction *txn = findTransaction(it->second);
        if (txn && txn->affectsBalance() && (!latest || *latest < *txn))
        {
            latest = txn;
        }
    }
    return latest;
}

// Total credits (deposits, incoming transfers, interest, loan disbursements, openings)
double TransactionManager::getTotalDeposits(string accountNumber, string startDate, string endDate) const
{
    double totalDeposits = 0.0;
    for (const Transaction &txn : getAccountHistory(accountNumber, -1))
    {
        if (txn.affectsBalance() && txn.isCredit() && inDateRange(txn, startDate, endDate))
        {
            totalDeposits += txn.getAmount();
        }
    }
    return roundMoney(totalDeposits);
}

// Total debits (withdrawals, outgoing transfers, fees, loan payments, closings)
double TransactionManager::getTotalWithdrawals(string accountNumber, string startDate, string endDate) const
{
    double totalWithdrawals = 0.0;
    for (const Transaction &txn : getAccountHistory(accountNumber, -1))
    {
        if (txn.affectsBalance() && !txn.isCredit() && txn.getTransactionType() != TransactionType::UNKNOWN &&
            inDateRange(txn, startDate, endDate))
        {
            totalWithdrawals += txn.getAmount();
        }
    }
    return roundMoney(totalWithdrawals);
}

// Get net flow for account
double TransactionManager::getNetFlow(string accountNumber, string startDate, string endDate) const
{
    return roundMoney(getTotalDeposits(accountNumber, startDate, endDate) -
                      getTotalWithdrawals(accountNumber, startDate, endDate));
}

// Get transaction count for account
int TransactionManager::getTransactionCount(string accountNumber, string startDate, string endDate) const
{
    return static_cast<int>(getAccountTransactionsByDateRange(accountNumber, startDate, endDate).size());
}

// Get average transaction amount by type
double TransactionManager::getAverageTransactionAmount(string accountNumber, TransactionType type) const
{
    double totalAmount = 0.0;
    int count = 0;
    for (const Transaction &txn : getAccountHistory(accountNumber, -1))
    {
        if (txn.getTransactionType() == type && txn.affectsBalance())
        {
            totalAmount += txn.getAmount();
            count++;
        }
    }
    return count > 0 ? roundMoney(totalAmount / count) : 0.0;
}

// Cash withdrawals made from an account on a given date (for the daily limit)
double TransactionManager::getDailyWithdrawalTotal(string accountNumber, string date) const
{
    double total = 0.0;
    for (const Transaction &txn : getAccountTransactionsByDateRange(accountNumber, date, date))
    {
        if (txn.getTransactionType() == TransactionType::WITHDRAWAL && txn.isSuccessful())
        {
            total += txn.getAmount();
        }
    }
    return roundMoney(total);
}

// Get current date and time
string TransactionManager::getCurrentDateTime() const
{
    return ::getCurrentDateTime();
}

// Get current date
string TransactionManager::getCurrentDate() const
{
    return ::getCurrentDate();
}

// Print a list of transactions as a table
void TransactionManager::displayTransactionTable(const vector<Transaction> &transactions)
{
    cout << left << setw(19) << "Transaction ID"
         << setw(21) << "Timestamp"
         << setw(18) << "Type"
         << right << setw(14) << "Amount"
         << setw(14) << "Balance"
         << "  " << left << "Description" << endl;
    cout << string(110, '-') << endl;
    for (const Transaction &txn : transactions)
    {
        string description = txn.getDescription();
        if (txn.getStatus() != TransactionStatus::COMPLETED)
        {
            description += " [" + txn.getStatusString() + "]";
        }
        cout << left << setw(19) << txn.getTransactionId()
             << setw(21) << txn.getTimestamp()
             << setw(18) << txn.getTransactionTypeString()
             << right << setw(14) << formatCurrency(txn.getNetAmount())
             << setw(14) << formatCurrency(txn.getBalanceAfter())
             << "  " << left << description << endl;
    }
    if (transactions.empty())
    {
        cout << "No transactions found." << endl;
    }
}

// Display transaction history for account
void TransactionManager::displayTransactionHistory(string accountNumber, int limit) const
{
    vector<Transaction> history = getAccountHistory(accountNumber, limit);
    cout << "Transaction History for Account " << accountNumber
         << " (latest " << history.size() << ")" << endl;
    cout << string(110, '=') << endl;
    displayTransactionTable(history);
    cout << string(110, '-') << endl;
    cout << "Total Credits : " << formatCurrency(getTotalDeposits(accountNumber)) << endl;
    cout << "Total Debits  : " << formatCurrency(getTotalWithdrawals(accountNumber)) << endl;
    cout << "Net Flow      : " << formatCurrency(getNetFlow(accountNumber)) << endl;
}

// Display customer transaction summary
void TransactionManager::displayCustomerTransactionSummary(string customerId) const
{
    cout << "\n=== Transaction Summary for Customer: " << customerId << " ===" << endl;

    vector<Transaction> customerTxns = getCustomerHistory(customerId, -1);
    if (customerTxns.empty())
    {
        cout << "No transactions found for this customer." << endl;
        return;
    }

    map<TransactionType, pair<int, double>> byType;
    double netFlow = 0.0;
    for (const auto &txn : customerTxns)
    {
        if (!txn.affectsBalance()) continue;
        byType[txn.getTransactionType()].first++;
        byType[txn.getTransactionType()].second += txn.getAmount();
        netFlow += txn.getNetAmount();
    }

    cout << "Total Transactions: " << customerTxns.size() << endl;
    for (const auto &[type, stats] : byType)
    {
        cout << "  " << left << setw(18) << Transaction::typeToString(type)
             << right << setw(5) << stats.first << "  " << setw(14) << formatCurrency(stats.second) << endl;
    }
    cout << "Net Flow: " << formatCurrency(netFlow) << endl;

    cout << "\nRecent Transactions (Last 5):" << endl;
    vector<Transaction> recent(customerTxns.begin(), customerTxns.begin() + min<size_t>(5, customerTxns.size()));
    displayTransactionTable(recent);
}

// Get total transaction count
int TransactionManager::getTotalTransactionCount() const
{
    return static_cast<int>(transactionHistory.size());
}

// Get total system volume (sum of all money moved by completed transactions)
double TransactionManager::getTotalSystemVolume() const
{
    double totalVolume = 0.0;
    for (const auto &pair : transactionHistory)
    {
        if (pair.second.affectsBalance())
        {
            totalVolume += pair.second.getAmount();
        }
    }
    return roundMoney(totalVolume);
}

// Display transactions by type
void TransactionManager::displayTransactionsByType(TransactionType type, int limit) const
{
    vector<Transaction> transactions = getTransactionsByType(type, limit);
    cout << "Transactions of Type: " << Transaction::typeToString(type) << endl;
    cout << string(110, '=') << endl;
    displayTransactionTable(transactions);
    double totalAmount = 0.0;
    for (const Transaction &txn : transactions)
    {
        totalAmount += txn.getAmount();
    }
    cout << string(110, '-') << endl;
    cout << "Total Transactions: " << transactions.size()
         << "   Total Amount: " << formatCurrency(totalAmount) << endl;
}

// Display daily transaction summary
void TransactionManager::displayDailyTransactionSummary(string date) const
{
    vector<Transaction> transactions = getTransactionsByDateRange(date, date);
    cout << "Daily Transaction Summary for " << date << endl;
    cout << string(60, '=') << endl;
    if (transactions.empty())
    {
        cout << "No transactions found for this date." << endl;
        return;
    }

    map<TransactionType, pair<int, double>> byType;
    double netFlow = 0.0;
    for (const auto &txn : transactions)
    {
        if (!txn.affectsBalance()) continue;
        byType[txn.getTransactionType()].first++;
        byType[txn.getTransactionType()].second += txn.getAmount();
        netFlow += txn.getNetAmount();
    }
    cout << left << setw(20) << "Type" << right << setw(8) << "Count" << setw(18) << "Amount" << endl;
    cout << string(60, '-') << endl;
    for (const auto &[type, stats] : byType)
    {
        cout << left << setw(20) << Transaction::typeToString(type)
             << right << setw(8) << stats.first << setw(18) << formatCurrency(stats.second) << endl;
    }
    cout << string(60, '-') << endl;
    cout << "Transactions: " << transactions.size() << "   Net Flow: " << formatCurrency(netFlow) << endl;
}

// Display transaction statistics
void TransactionManager::displayTransactionStatistics() const
{
    int completed = 0, failed = 0, reversed = 0, pending = 0;
    map<TransactionType, int> typeCount;
    for (const auto &pair : transactionHistory)
    {
        const Transaction &txn = pair.second;
        typeCount[txn.getTransactionType()]++;
        switch (txn.getStatus())
        {
        case TransactionStatus::COMPLETED: completed++; break;
        case TransactionStatus::FAILED:    failed++; break;
        case TransactionStatus::REVERSED:  reversed++; break;
        case TransactionStatus::PENDING:   pending++; break;
        default: break;
        }
    }
    cout << "Transaction Statistics" << endl;
    cout << string(40, '-') << endl;
    cout << "Total Transactions : " << transactionHistory.size() << endl;
    cout << "Total Volume       : " << formatCurrency(getTotalSystemVolume()) << endl;
    cout << "Completed          : " << completed << endl;
    cout << "Reversed           : " << reversed << endl;
    cout << "Failed             : " << failed << endl;
    cout << "Pending            : " << pending << endl;
    cout << string(40, '-') << endl;
    for (const auto &pair : typeCount)
    {
        cout << "  " << left << setw(18) << Transaction::typeToString(pair.first) << right << setw(6) << pair.second << endl;
    }
}

// Save transaction history to file (written to a temp file, then renamed)
bool TransactionManager::saveTransactionHistory() const
{
    try
    {
        fs::path target(logFilePath);
        if (target.has_parent_path())
        {
            fs::create_directories(target.parent_path());
        }
        string tempPath = logFilePath + ".tmp";
        {
            ofstream file(tempPath);
            if (!file.is_open())
            {
                return false;
            }
            file << "# id|account|related|type|amount|before|after|timestamp|description|status|customer|session|notes|linked" << endl;
            for (const auto &pair : transactionHistory)
            {
                const Transaction &txn = pair.second;
                stringstream amount, before, after;
                amount << fixed << setprecision(2) << txn.getAmount();
                before << fixed << setprecision(2) << txn.getBalanceBefore();
                after << fixed << setprecision(2) << txn.getBalanceAfter();
                file << joinRecord({txn.getTransactionId(), txn.getAccountNumber(), txn.getRelatedAccountNumber(),
                                    txn.getTransactionTypeString(), amount.str(), before.str(), after.str(),
                                    txn.getTimestamp(), txn.getDescription(), txn.getStatusString(),
                                    txn.getCustomerId(), txn.getSessionId(), txn.getNotes(),
                                    txn.getLinkedTransactionId()})
                     << '\n';
            }
            if (!file.good())
            {
                return false;
            }
        }
        fs::rename(tempPath, logFilePath);
        return true;
    }
    catch (const exception &)
    {
        return false;
    }
}

// Load transaction history from file (replaces anything in memory)
bool TransactionManager::loadTransactionHistory()
{
    ifstream file(logFilePath);
    if (!file.is_open())
    {
        return false; // No saved history yet
    }
    clear();
    string line;
    while (getline(file, line))
    {
        if (trim(line).empty() || line[0] == '#')
        {
            continue;
        }
        vector<string> f = splitRecord(line);
        if (f.size() < 14 || f[0].empty())
        {
            continue; // Skip malformed lines
        }
        Transaction transaction(f[0], f[1], Transaction::typeFromString(f[3]), parseDouble(f[4]),
                                f[8], parseDouble(f[5]), parseDouble(f[6]), f[10]);
        transaction.setRelatedAccountNumber(f[2]);
        transaction.setTimestamp(f[7]);
        transaction.setStatus(Transaction::statusFromString(f[9]));
        transaction.setSessionId(f[11]);
        transaction.setNotes(f[12]);
        transaction.setLinkedTransactionId(f[13]);
        addTransaction(transaction);
    }
    return true;
}

// Export transactions to CSV
bool TransactionManager::exportTransactionsToCSV(string filename, string accountNumber) const
{
    try
    {
        fs::path target(filename);
        if (target.has_parent_path())
        {
            fs::create_directories(target.parent_path());
        }
    }
    catch (const exception &)
    {
        return false;
    }
    ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    file << "ID,Timestamp,Account,Related Account,Type,Amount,Balance Before,Balance After,Status,Description" << endl;
    vector<Transaction> transactions = accountNumber.empty() ? getAllTransactions() : getAccountHistory(accountNumber, -1);
    reverse(transactions.begin(), transactions.end()); // Chronological order for statements
    for (const auto &txn : transactions)
    {
        file << txn.getTransactionId() << ","
             << txn.getTimestamp() << ","
             << txn.getAccountNumber() << ","
             << txn.getRelatedAccountNumber() << ","
             << txn.getTransactionTypeString() << ","
             << fixed << setprecision(2) << txn.getNetAmount() << ","
             << txn.getBalanceBefore() << ","
             << txn.getBalanceAfter() << ","
             << txn.getStatusString() << ","
             << csvField(txn.getDescription()) << endl;
    }
    return file.good();
}

// Create backup of transaction data
bool TransactionManager::createBackup(string backupPath) const
{
    try
    {
        string stamp = ::getCurrentDateTime();
        replace(stamp.begin(), stamp.end(), ' ', '_');
        replace(stamp.begin(), stamp.end(), ':', '-');
        fs::create_directories(backupPath);
        fs::path backupFile = fs::path(backupPath) / ("transactions_" + stamp + ".dat");
        fs::copy_file(logFilePath, backupFile, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const exception &)
    {
        return false;
    }
}

void TransactionManager::setLogFilePath(string path)
{
    logFilePath = path;
}

string TransactionManager::getLogFilePath() const
{
    return logFilePath;
}

// Remove all transactions from memory
void TransactionManager::clear()
{
    transactionHistory.clear();
    accountTransactions.clear();
    customerTransactions.clear();
    dateTransactions.clear();
    nextTransactionNumber = 1;
}

// Generate unique transaction ID: TXN + YYYYMMDD + 6-digit global sequence
string TransactionManager::generateTransactionId()
{
    string dateStr = ::getCurrentDate();
    dateStr.erase(remove(dateStr.begin(), dateStr.end(), '-'), dateStr.end());

    stringstream ss;
    ss << "TXN" << dateStr << setfill('0') << setw(6) << nextTransactionNumber++;
    return ss.str();
}

// Validate transaction ID format: TXN + 8-digit date + 6+ digit sequence
bool TransactionManager::isValidTransactionId(string transactionId) const
{
    if (transactionId.size() < 17 || transactionId.compare(0, 3, "TXN") != 0)
    {
        return false;
    }
    for (size_t i = 3; i < transactionId.size(); i++)
    {
        if (!isdigit(static_cast<unsigned char>(transactionId[i])))
        {
            return false;
        }
    }
    string date = transactionId.substr(3, 4) + "-" + transactionId.substr(7, 2) + "-" + transactionId.substr(9, 2);
    return isValidDate(date);
}

// Remove transactions older than daysOld days; returns how many were removed
int TransactionManager::cleanupOldTransactions(int daysOld)
{
    string cutoffDate = addDaysToDate(::getCurrentDate(), -daysOld);
    int removedCount = 0;
    for (auto it = transactionHistory.begin(); it != transactionHistory.end();)
    {
        if (it->second.getDate() < cutoffDate)
        {
            it = transactionHistory.erase(it);
            removedCount++;
        }
        else
        {
            ++it;
        }
    }
    if (removedCount > 0)
    {
        reindexTransactions();
    }
    return removedCount;
}

// Rebuild transaction indexes from the main history map
void TransactionManager::reindexTransactions()
{
    accountTransactions.clear();
    customerTransactions.clear();
    dateTransactions.clear();
    for (const auto &pair : transactionHistory)
    {
        indexTransaction(pair.second);
        updateNextTransactionNumber(pair.first);
    }
}
