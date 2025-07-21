#include "headers/Account.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include "headers/Utilities.h" 
using namespace std;

// =============================================================================
// BASE ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for Account class
Account::Account(string accNum, string custId, double initialBalance, AccountType type) {
    accountNumber=accNum;
    customerId=custId;
    if(initialBalance<0)
    {
        cout<<"Invalid InitialBalance(<0), default set to 0"<<endl;
        balance=0;
    }
    else balance=initialBalance;
    accountType=type;
    isActive=true;
    creationDate=getCurrentDate();
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

// Setter: Set account active status
void Account::setActive(bool status) {
    isActive=status;
    if(status) {
        cout<<"Account is now Active"<<endl;
    } else {
        cout<<"Account is now Inactive"<<endl;
    }
}

// Setter: Update account balance
void Account::updateBalance(double newBalance) {
    if(newBalance<0) cout<<"Your Balance is Now Negative"<<endl;
    balance=newBalance;
}

// =============================================================================
// SAVINGS ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for SavingsAccount
SavingsAccount::SavingsAccount(string accNum, string custId, double initialBalance, double intRate)
    : Account(accNum, custId, initialBalance, AccountType::SAVINGS) {
    interestRate=intRate;
    minimumBalance=500;
    withdrawalCount=0;
}

// Deposit money to savings account
void SavingsAccount::deposit(double amount) {
    if(amount<0) 
    {
        cout<<"Invalid Deposit Amount(<0)"<<endl;
        return;
    }
    double bal=getBalance();
    bal+=amount;
    updateBalance(bal);
    cout<<amount<<" Added to the Balance Successfully"<<endl;
}

// Withdraw money from savings account
bool SavingsAccount::withdraw(double amount) {
    if(amount<0) 
    {
        cout<<"Invalid Withdrawal Amount(<0)"<<endl;
        return false;
    }
    if(getRemainingWithdrawals()<=0) 
    {
        cout<<"max Withdrawal count reached, reset it to withdraw"<<endl;
        return false;
    }
    double bal=getBalance();
    if(bal-amount<minimumBalance)
    {
        cout<<"this withrawal will cause the account balance go below min. balance set for your account. Withdrawal Failed!"<<endl;
        return false;
    }
    bal-=amount;
    updateBalance(bal);
    cout<<"Withdrawal Successful"<<endl;
    withdrawalCount++;
    return true;
}

// Display savings account information
void SavingsAccount::displayAccountInfo() const {
    cout<<"Account Number : "<<getAccountNumber()<<endl;
    AccountType at=getAccountType();
    string act="";
    if(at==AccountType::SAVINGS) act="Savings";
    else if(at==AccountType::CHECKING) act="Checking";
    else act="Unknown";
    cout<<"Account Type : "<< act<<endl;
    cout<<"Balance : "<<fixed<<setprecision(2)<<getBalance()<<endl;
    cout << "Interest Rate : " << fixed << setprecision(2) << (getInterestRate() * 100) << "%" << endl;
    cout<<"Remaining Withdrawals : "<<getRemainingWithdrawals()<<endl;
    cout<<"Minimum Balance : "<<fixed<<setprecision(2)<<minimumBalance<<endl;
    cout<<"Status : "<<(isAccountActive() ? "Active" : "Inactive")<<endl;
    cout<<"Creation Date : "<<getCreationDate()<<endl;
}

// Calculate interest for savings account
double SavingsAccount::calculateInterest() const {
    double interest = getBalance() * (interestRate / 100.0) / 12.0;
    return interest;
}

// Apply interest to account balance
void SavingsAccount::applyInterest() {
    double interest = calculateInterest();
    if(interest<0)
    {
        cout<<"Interest is Negative, not applied"<<endl;
        return;
    }
    double bal = getBalance();
    bal += interest;
    updateBalance(bal);
    cout<<"Interest of "<<fixed<<setprecision(2)<<interest<<" applied successfully"<<endl;
    cout<<"New Balance : "<<fixed<<setprecision(2)<<getBalance()<<endl;
    return;
}

// Get interest rate
double SavingsAccount::getInterestRate() const {
    return interestRate;
}

// Get remaining withdrawals for current month
int SavingsAccount::getRemainingWithdrawals() const {
    return MAX_WITHDRAWALS-withdrawalCount;
}

// Reset withdrawal count (called monthly)
void SavingsAccount::resetWithdrawalCount() {
    withdrawalCount=0;
    return ;
}

// =============================================================================
// CHECKING ACCOUNT CLASS IMPLEMENTATIONS
// =============================================================================

// Constructor for CheckingAccount
CheckingAccount::CheckingAccount(string accNum, string custId, double initialBalance, double overdraft)
    : Account(accNum, custId, initialBalance, AccountType::CHECKING) {
    overdraftLimit = overdraft;
    transactionFee = 2.50;
    freeTransactions = 10;
    transactionCount = 0;
    if (overdraftLimit < 0) {
        cout << "Invalid overdraft limit (< 0), default set to 0" << endl;
        overdraftLimit = 0;
    }
}

// Deposit money to checking account
void CheckingAccount::deposit(double amount) {
    if (amount < 0) {
        cout << "Invalid Deposit Amount (< 0)" << endl;
        return;
    }
    double bal = getBalance();
    bal += amount;
    updateBalance(bal);
    transactionCount++;
    cout << fixed << setprecision(2) << amount << " added to the balance successfully" << endl;
    if (transactionCount > freeTransactions) {
        chargeFee();
    }
}

// Withdraw money from checking account
bool CheckingAccount::withdraw(double amount) {
    if (amount < 0) {
        cout << "Invalid Withdrawal Amount (< 0)" << endl;
        return false;
    }
    double availableBalance = getAvailableBalance();
    if (amount > availableBalance) {
        cout << "Insufficient funds to withdraw " << fixed << setprecision(2) << amount << endl;
        return false;
    }
    double bal = getBalance();
    bal -= amount;
    updateBalance(bal);
    transactionCount++;
    cout << fixed << setprecision(2) << amount << " withdrawn successfully" << endl;
    if (transactionCount > freeTransactions) {
        chargeFee();
    }
    return true;
}

// Display checking account information
void CheckingAccount::displayAccountInfo() const {
    cout << "Account Number: " << getAccountNumber() << endl;
    AccountType at = getAccountType();
    string act = (at == AccountType::CHECKING) ? "Checking" : "Unknown";
    cout << "Account Type: " << act << endl;
    cout << "Balance: " << fixed << setprecision(2) << getBalance() << endl;
    cout << "Available Balance: " << fixed << setprecision(2) << getAvailableBalance() << endl;
    cout << "Overdraft Limit: " << fixed << setprecision(2) << getOverdraftLimit() << endl;
    cout << "Transaction Count: " << transactionCount << endl;
    cout << "Status: " << (isAccountActive() ? "Active" : "Inactive") << endl;
    cout << "Creation Date: " << getCreationDate() << endl;
}

// Get available balance (including overdraft)
double CheckingAccount::getAvailableBalance() const {
    double bal = getBalance();
    return bal + overdraftLimit;
}

// Set overdraft limit
void CheckingAccount::setOverdraftLimit(double limit) {
    if (limit < 0) {
        cout << "Invalid overdraft limit (< 0), not set." << endl;
        return;
    }
    overdraftLimit = limit;
    cout << "Overdraft limit set to " << fixed << setprecision(2) << overdraftLimit << endl;
}

// Get overdraft limit
double CheckingAccount::getOverdraftLimit() const {
    return overdraftLimit;
}

// Charge transaction fee
void CheckingAccount::chargeFee() {
    if (transactionCount > freeTransactions) {
        double bal = getBalance();
        bal -= transactionFee;
        if (bal < 0) {
            cout << "Insufficient funds to charge transaction fee." << endl;
            return;
        }
        updateBalance(bal);
        cout << "Transaction fee of " << fixed << setprecision(2) << transactionFee << " charged successfully." << endl;
    }
}

// Reset transaction count (called monthly)
void CheckingAccount::resetTransactionCount() {
    transactionCount = 0;
    cout << "Transaction count reset to 0." << endl;

}
