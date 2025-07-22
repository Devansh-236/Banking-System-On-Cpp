#include <iostream>
#include "Account.h"
#include "Customer.h"
#include "Transaction.h"
#include<iomanip>

using namespace std;

int main() {
    cout << "=== Banking System Transaction Testing ===" << endl;
    
    // Initialize transaction manager
    TransactionManager txnManager("data/test_transactions.log");
    
    // Create test customer and accounts
    Customer customer1("CUST001", "John Doe", "123 Main St", "555-0123", "john@email.com", "1234");
    Account* savings = new SavingsAccount("SAV001", "CUST001", 1000.0);
    Account* checking = new CheckingAccount("CHK001", "CUST001", 500.0);
    
    customer1.addAccount(savings);
    customer1.addAccount(checking);
    
    cout << "\n--- Testing Transaction Processing ---" << endl;
    
    // Test 1: Process Deposit
    double oldBalance = savings->getBalance();
    savings->deposit(200.0);
    double newBalance = savings->getBalance();
    
    string txnId1 = txnManager.processDeposit("SAV001", 200.0, "Cash Deposit", 
                                              oldBalance, newBalance, "CUST001");
    cout << "Deposit Transaction ID: " << txnId1 << endl;
    
    // Test 2: Process Withdrawal
    oldBalance = checking->getBalance();
    checking->withdraw(100.0);
    newBalance = checking->getBalance();
    
    string txnId2 = txnManager.processWithdrawal("CHK001", 100.0, "ATM Withdrawal",
                                                 oldBalance, newBalance, "CUST001");
    cout << "Withdrawal Transaction ID: " << txnId2 << endl;
    
    // Test 3: Process Transfer
    double savingsOldBal = savings->getBalance();
    double checkingOldBal = checking->getBalance();
    
    // Simulate transfer: $150 from savings to checking
    savings->withdraw(150.0);
    checking->deposit(150.0);
    
    string txnId3 = txnManager.processTransfer("SAV001", "CHK001", 150.0, "Internal Transfer",
                                               savingsOldBal, savings->getBalance(),
                                               checkingOldBal, checking->getBalance(), "CUST001");
    cout << "Transfer Transaction ID: " << txnId3 << endl;
    
    // Test 4: Display Transaction History
    cout << "\n--- Transaction History ---" << endl;
    txnManager.displayTransactionHistory("SAV001", 10);
    
    cout << "\n--- Customer Transaction Summary ---" << endl;
    txnManager.displayCustomerTransactionSummary("CUST001");
    
    // Test 5: Transaction Analytics
    cout << "\n--- Transaction Analytics ---" << endl;
    cout << "Total Deposits (SAV001): $" << fixed << setprecision(2) 
         << txnManager.getTotalDeposits("SAV001") << endl;
    cout << "Total Withdrawals (SAV001): $" << fixed << setprecision(2)
         << txnManager.getTotalWithdrawals("SAV001") << endl;
    cout << "Net Flow (SAV001): $" << fixed << setprecision(2)
         << txnManager.getNetFlow("SAV001") << endl;
    
    // Test 6: Transaction Search
    cout << "\n--- Transaction Search ---" << endl;
    vector<Transaction> deposits = txnManager.getTransactionsByType(TransactionType::DEPOSIT);
    cout << "Found " << deposits.size() << " deposit transactions" << endl;
    
    // Test 7: System Statistics
    cout << "\n--- System Statistics ---" << endl;
    txnManager.displayTransactionStatistics();
    
    // Test 8: Save Transaction Data
    cout << "\n--- Saving Transaction Data ---" << endl;
    if (txnManager.saveTransactionHistory()) {
        cout << "Transaction history saved successfully!" << endl;
    } else {
        cout << "Failed to save transaction history!" << endl;
    }
    
    cout << "\n=== Transaction Testing Complete ===" << endl;
    
    return 0;
}
