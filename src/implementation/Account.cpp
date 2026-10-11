#include "Account.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "Utilities.h"
using namespace std;

// =============================================================================
// ACCOUNT TYPE HELPERS
// =============================================================================

string accountTypeToString(AccountType type) {
    return type == AccountType::SAVINGS ? "SAVINGS" : "CHECKING";
}

bool accountTypeFromString(const string& name, AccountType& type) {
    string upper = toUpper(trim(name));
    if (upper == "SAVINGS") { type = AccountType::SAVINGS; return true; }
    if (upper == "CHECKING") { type = AccountType::CHECKING; return true; }
    return false;
}

// =============================================================================
// BASE ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for Account class
Account::Account(string accNum, string custId, double initialBalance, AccountType type) {
    accountNumber = accNum;
    customerId = custId;
    balance = initialBalance < 0 ? 0 : roundMoney(initialBalance);
    accountType = type;
    isActive = true;
    creationDate = getCurrentDate();
}

// Getter: Return account number
string Account::getAccountNumber() const {
    return accountNumber;
}

// Getter: Return current balance
double Account::getBalance() const {
    return balance;
}

// Getter: Return customer ID
string Account::getCustomerId() const {
    return customerId;
}

// Getter: Check if account is active
bool Account::isAccountActive() const {
    return isActive;
}

// Getter: Return creation date
string Account::getCreationDate() const {
    return creationDate;
}

// Getter: Return account type
AccountType Account::getAccountType() const {
    return accountType;
}

// Getter: Return account type as display string
string Account::getAccountTypeString() const {
    return accountTypeToString(accountType);
}

// Setter: Set account active status
void Account::setActive(bool status) {
    isActive = status;
}

// Setter: Update account balance
void Account::updateBalance(double newBalance) {
    balance = roundMoney(newBalance);
}

// Setter: Change owning customer (used for ownership transfers)
void Account::setCustomerId(string custId) {
    customerId = custId;
}

// Setter: Restore creation date (used when loading saved data)
void Account::setCreationDate(string date) {
    creationDate = date;
}

// =============================================================================
// SAVINGS ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for SavingsAccount
SavingsAccount::SavingsAccount(string accNum, string custId, double initialBalance,
                               double intRate, double minBalance, int maxMonthlyWithdrawals)
    : Account(accNum, custId, initialBalance, AccountType::SAVINGS) {
    interestRate = intRate < 0 ? 0 : intRate;
    minimumBalance = minBalance < 0 ? 0 : minBalance;
    withdrawalCount = 0;
    maxWithdrawals = maxMonthlyWithdrawals < 0 ? 0 : maxMonthlyWithdrawals;
}

// Deposit money to savings account
bool SavingsAccount::deposit(double amount) {
    if (!isActive || !isValidAmount(amount)) {
        return false;
    }
    updateBalance(balance + amount);
    return true;
}

// Check whether a withdrawal is allowed
bool SavingsAccount::canWithdraw(double amount, string& reason) const {
    if (!isActive) {
        reason = "Account " + accountNumber + " is frozen/inactive.";
        return false;
    }
    if (!isValidAmount(amount)) {
        reason = "Withdrawal amount must be greater than zero.";
        return false;
    }
    if (getRemainingWithdrawals() <= 0) {
        reason = "Monthly withdrawal limit of " + to_string(maxWithdrawals) +
                 " reached for savings account " + accountNumber + ".";
        return false;
    }
    if (roundMoney(balance - amount) < minimumBalance) {
        reason = "Withdrawal would take the balance below the minimum of " +
                 formatCurrency(minimumBalance) + ". Maximum you can withdraw: " +
                 formatCurrency(max(0.0, balance - minimumBalance)) + ".";
        return false;
    }
    return true;
}

// Withdraw money from savings account
bool SavingsAccount::withdraw(double amount) {
    string reason;
    if (!canWithdraw(amount, reason)) {
        return false;
    }
    updateBalance(balance - amount);
    withdrawalCount++;
    return true;
}

// Lowest balance a savings account may reach through customer withdrawals
double SavingsAccount::getLowestAllowedBalance() const {
    return minimumBalance;
}

// Display savings account information
void SavingsAccount::displayAccountInfo() const {
    cout << "Account Number       : " << accountNumber << endl;
    cout << "Account Type         : Savings" << endl;
    cout << "Balance              : " << formatCurrency(balance) << endl;
    cout << "Interest Rate        : " << fixed << setprecision(2) << (interestRate * 100) << "% p.a." << endl;
    cout << "Minimum Balance      : " << formatCurrency(minimumBalance) << endl;
    cout << "Withdrawals Left     : " << getRemainingWithdrawals() << " of " << maxWithdrawals << " this month" << endl;
    cout << "Status               : " << (isActive ? "Active" : "Frozen") << endl;
    cout << "Creation Date        : " << creationDate << endl;
}

// Calculate one month of interest at the annual rate
double SavingsAccount::calculateInterest() const {
    if (balance <= 0) return 0.0;
    return roundMoney(balance * interestRate / 12.0);
}

// Apply interest to account balance; returns the amount credited
double SavingsAccount::applyInterest() {
    if (!isActive) return 0.0;
    double interest = calculateInterest();
    if (interest <= 0) return 0.0;
    updateBalance(balance + interest);
    return interest;
}

// Get interest rate
double SavingsAccount::getInterestRate() const {
    return interestRate;
}

void SavingsAccount::setInterestRate(double rate) {
    if (rate >= 0) interestRate = rate;
}

double SavingsAccount::getMinimumBalance() const {
    return minimumBalance;
}

void SavingsAccount::setMinimumBalance(double minBalance) {
    if (minBalance >= 0) minimumBalance = minBalance;
}

int SavingsAccount::getWithdrawalCount() const {
    return withdrawalCount;
}

void SavingsAccount::setWithdrawalCount(int count) {
    withdrawalCount = count < 0 ? 0 : count;
}

int SavingsAccount::getMaxWithdrawals() const {
    return maxWithdrawals;
}

void SavingsAccount::setMaxWithdrawals(int count) {
    if (count >= 0) maxWithdrawals = count;
}

// Get remaining withdrawals for current month
int SavingsAccount::getRemainingWithdrawals() const {
    return max(0, maxWithdrawals - withdrawalCount);
}

// Reset withdrawal count (called monthly)
void SavingsAccount::resetWithdrawalCount() {
    withdrawalCount = 0;
}

// =============================================================================
// CHECKING ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for CheckingAccount
CheckingAccount::CheckingAccount(string accNum, string custId, double initialBalance, double overdraft,
                                 double fee, int freeMonthlyTransactions)
    : Account(accNum, custId, initialBalance, AccountType::CHECKING) {
    overdraftLimit = overdraft < 0 ? 0 : overdraft;
    transactionFee = fee < 0 ? 0 : fee;
    freeTransactions = freeMonthlyTransactions < 0 ? 0 : freeMonthlyTransactions;
    transactionCount = 0;
}

// Deposit money to checking account
bool CheckingAccount::deposit(double amount) {
    if (!isActive || !isValidAmount(amount)) {
        return false;
    }
    updateBalance(balance + amount);
    transactionCount++;
    return true;
}

// Check whether a withdrawal is allowed
bool CheckingAccount::canWithdraw(double amount, string& reason) const {
    if (!isActive) {
        reason = "Account " + accountNumber + " is frozen/inactive.";
        return false;
    }
    if (!isValidAmount(amount)) {
        reason = "Withdrawal amount must be greater than zero.";
        return false;
    }
    if (roundMoney(amount) > roundMoney(getAvailableBalance())) {
        reason = "Insufficient funds. Available balance (including overdraft): " +
                 formatCurrency(max(0.0, getAvailableBalance())) + ".";
        return false;
    }
    return true;
}

// Withdraw money from checking account
bool CheckingAccount::withdraw(double amount) {
    string reason;
    if (!canWithdraw(amount, reason)) {
        return false;
    }
    updateBalance(balance - amount);
    transactionCount++;
    return true;
}

// Lowest balance a checking account may reach (into the overdraft)
double CheckingAccount::getLowestAllowedBalance() const {
    return -overdraftLimit;
}

// Display checking account information
void CheckingAccount::displayAccountInfo() const {
    cout << "Account Number       : " << accountNumber << endl;
    cout << "Account Type         : Checking" << endl;
    cout << "Balance              : " << formatCurrency(balance) << endl;
    cout << "Available Balance    : " << formatCurrency(getAvailableBalance()) << endl;
    cout << "Overdraft Limit      : " << formatCurrency(overdraftLimit) << endl;
    cout << "Transactions (month) : " << transactionCount << " (" << freeTransactions << " free, then "
         << formatCurrency(transactionFee) << " each)" << endl;
    cout << "Status               : " << (isActive ? "Active" : "Frozen") << endl;
    cout << "Creation Date        : " << creationDate << endl;
}

// Get available balance (including overdraft)
double CheckingAccount::getAvailableBalance() const {
    return roundMoney(balance + overdraftLimit);
}

// Set overdraft limit
void CheckingAccount::setOverdraftLimit(double limit) {
    if (limit >= 0) overdraftLimit = limit;
}

// Get overdraft limit
double CheckingAccount::getOverdraftLimit() const {
    return overdraftLimit;
}

double CheckingAccount::getTransactionFee() const {
    return transactionFee;
}

void CheckingAccount::setTransactionFee(double fee) {
    if (fee >= 0) transactionFee = fee;
}

int CheckingAccount::getFreeTransactions() const {
    return freeTransactions;
}

void CheckingAccount::setFreeTransactions(int count) {
    if (count >= 0) freeTransactions = count;
}

int CheckingAccount::getTransactionCount() const {
    return transactionCount;
}

void CheckingAccount::setTransactionCount(int count) {
    transactionCount = count < 0 ? 0 : count;
}

// Fee owed for transactions beyond the free allowance this month
double CheckingAccount::calculateMonthlyFee() const {
    int billable = max(0, transactionCount - freeTransactions);
    return roundMoney(billable * transactionFee);
}

// Charge the monthly transaction fee (never beyond the overdraft limit); returns amount charged
double CheckingAccount::chargeFee() {
    double fee = min(calculateMonthlyFee(), max(0.0, getAvailableBalance()));
    fee = roundMoney(fee);
    if (fee > 0) {
        updateBalance(balance - fee);
    }
    return fee;
}

// Reset transaction count (called monthly)
void CheckingAccount::resetTransactionCount() {
    transactionCount = 0;
}
