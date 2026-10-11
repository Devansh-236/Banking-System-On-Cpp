#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <iostream>

using namespace std;

// Enumeration for account types
enum class AccountType {
    SAVINGS,
    CHECKING
};

// Convert an account type to/from its display name
string accountTypeToString(AccountType type);
bool accountTypeFromString(const string& name, AccountType& type);

// Abstract base class for all account types.
// Accounts never print; they report failures through canWithdraw()/return values
// and leave all user-facing messages to the Bank and UI layers.
class Account {
protected:
    string accountNumber;
    double balance;
    AccountType accountType;
    string creationDate;
    bool isActive;
    string customerId;

public:
    // Constructor
    Account(string accNum, string custId, double initialBalance, AccountType type);

    // Pure virtual functions (must be implemented by derived classes)
    virtual bool deposit(double amount) = 0;
    virtual bool withdraw(double amount) = 0;
    virtual void displayAccountInfo() const = 0;

    // Returns true if the withdrawal is allowed; otherwise fills reason
    virtual bool canWithdraw(double amount, string& reason) const = 0;

    // Lowest balance this account may reach (0 / minimum balance / -overdraft)
    virtual double getLowestAllowedBalance() const = 0;

    // Getter functions
    string getAccountNumber() const;
    double getBalance() const;
    string getCustomerId() const;
    bool isAccountActive() const;
    string getCreationDate() const;
    AccountType getAccountType() const;
    string getAccountTypeString() const;

    // Setter functions
    void setActive(bool status);
    void updateBalance(double newBalance);
    void setCustomerId(string custId);
    void setCreationDate(string date);

    // Virtual destructor
    virtual ~Account() = default;
};

// Derived class: Savings Account
class SavingsAccount : public Account {
private:
    double interestRate;        // Annual rate, e.g. 0.04 for 4%
    double minimumBalance;
    int withdrawalCount;        // Withdrawals made this month
    int maxWithdrawals;         // Withdrawals allowed per month

public:
    // Constructor
    SavingsAccount(string accNum, string custId, double initialBalance,
                   double intRate = 0.04, double minBalance = 500.0, int maxMonthlyWithdrawals = 6);

    // Override virtual functions
    bool deposit(double amount) override;
    bool withdraw(double amount) override;
    void displayAccountInfo() const override;
    bool canWithdraw(double amount, string& reason) const override;
    double getLowestAllowedBalance() const override;

    // Savings-specific functions
    double calculateInterest() const;   // One month of interest at the annual rate
    double applyInterest();             // Credits interest, returns amount credited
    double getInterestRate() const;
    void setInterestRate(double rate);
    double getMinimumBalance() const;
    void setMinimumBalance(double minBalance);
    int getWithdrawalCount() const;
    void setWithdrawalCount(int count);
    int getMaxWithdrawals() const;
    void setMaxWithdrawals(int count);
    int getRemainingWithdrawals() const;
    void resetWithdrawalCount();
};

// Derived class: Checking Account
class CheckingAccount : public Account {
private:
    double overdraftLimit;
    double transactionFee;      // Charged per transaction beyond the free allowance
    int freeTransactions;       // Free transactions per month
    int transactionCount;       // Transactions made this month

public:
    // Constructor
    CheckingAccount(string accNum, string custId, double initialBalance, double overdraft = 500.0,
                    double fee = 2.50, int freeMonthlyTransactions = 10);

    // Override virtual functions
    bool deposit(double amount) override;
    bool withdraw(double amount) override;
    void displayAccountInfo() const override;
    bool canWithdraw(double amount, string& reason) const override;
    double getLowestAllowedBalance() const override;

    // Checking-specific functions
    double getAvailableBalance() const;
    void setOverdraftLimit(double limit);
    double getOverdraftLimit() const;
    double getTransactionFee() const;
    void setTransactionFee(double fee);
    int getFreeTransactions() const;
    void setFreeTransactions(int count);
    int getTransactionCount() const;
    void setTransactionCount(int count);
    double calculateMonthlyFee() const; // Fee owed for transactions beyond the free allowance
    double chargeFee();                 // Deducts the monthly fee, returns amount charged
    void resetTransactionCount();
};

#endif // ACCOUNT_H
