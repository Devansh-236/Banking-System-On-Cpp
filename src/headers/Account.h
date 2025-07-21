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

// Abstract base class for all account types
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
    virtual void deposit(double amount) = 0;
    virtual bool withdraw(double amount) = 0;
    virtual void displayAccountInfo() const = 0;
    
    // Getter functions
    string getAccountNumber() const;
    double getBalance() const;
    string getCustomerId() const;
    bool isAccountActive() const;
    string getCreationDate() const;
    AccountType getAccountType() const;
    
    // Setter functions
    void setActive(bool status);
    void updateBalance(double newBalance);
    
    // Virtual destructor
    virtual ~Account() = default;
};

// Derived class: Savings Account
class SavingsAccount : public Account {
private:
    double interestRate;
    double minimumBalance;
    int withdrawalCount;
    static const int MAX_WITHDRAWALS = 20;

public:
    // Constructor
    SavingsAccount(string accNum, string custId, double initialBalance, double intRate = 0.04);
    
    // Override virtual functions
    void deposit(double amount) override;
    bool withdraw(double amount) override;
    void displayAccountInfo() const override;
    
    // Savings-specific functions
    double calculateInterest() const;
    void applyInterest();
    double getInterestRate() const;
    int getRemainingWithdrawals() const;
    void resetWithdrawalCount();
};

// Derived class: Checking Account
class CheckingAccount : public Account {
private:
    double overdraftLimit;
    double transactionFee;
    int freeTransactions;
    int transactionCount;

public:
    // Constructor
    CheckingAccount(string accNum, string custId, double initialBalance, double overdraft = 500.0);
    
    // Override virtual functions
    void deposit(double amount) override;
    bool withdraw(double amount) override;
    void displayAccountInfo() const override;
    
    // Checking-specific functions
    double getAvailableBalance() const;
    void setOverdraftLimit(double limit);
    double getOverdraftLimit() const;
    void chargeFee();
    void resetTransactionCount();
};

#endif // ACCOUNT_H
