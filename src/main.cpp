#include <iostream>
#include "headers/Account.h"

int main() {
    cout << "=== Banking System Account Testing ===" << endl;
    
    // Test 1: Create Savings Account
    cout << "\n--- Testing Savings Account ---" << endl;
    SavingsAccount savings("SAV001", "CUST001", 1000.0);
    savings.displayAccountInfo();
    
    cout << "\nDepositing $200..." << endl;
    savings.deposit(200.0);
    
    cout << "\nWithdrawing $100..." << endl;
    savings.withdraw(100.0);
    
    cout << "\nApplying interest..." << endl;
    savings.applyInterest();
    
    // Test 2: Create Checking Account
    cout << "\n--- Testing Checking Account ---" << endl;
    CheckingAccount checking("CHK001", "CUST001", 500.0);
    checking.displayAccountInfo();
    
    cout << "\nDepositing $100..." << endl;
    checking.deposit(100.0);
    
    cout << "\nWithdrawing $700 (testing overdraft)..." << endl;
    checking.withdraw(700.0);
    
    cout << "\nFinal account states:" << endl;
    savings.displayAccountInfo();
    cout << endl;
    checking.displayAccountInfo();
    
    return 0;
}
