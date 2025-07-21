#include <iostream>
#include "headers/Account.h"
#include "headers/Customer.h"
#include "headers/Utilities.h"
#include <iomanip>

using namespace std;

int main() {
    cout << "=== Banking System Customer Testing ===" << endl;
    
    // Test 1: Create Customer
    cout << "\n--- Creating Customer ---" << endl;
    Customer customer1("CUST001", "John Doe", "123 Main St", "555-0123", "john@email.com", "1234");
    customer1.displayCustomerInfo();
    
    // Test 2: Create and Add Accounts
    cout << "\n--- Adding Accounts ---" << endl;
    Account* savings1 = new SavingsAccount("SAV001", "CUST001", 1000.0);
    Account* checking1 = new CheckingAccount("CHK001", "CUST001", 500.0);
    
    customer1.addAccount(savings1);
    customer1.addAccount(checking1);
    
    // Test 3: Display Account Summary
    cout << "\n--- Account Summary ---" << endl;
    customer1.displayAccountSummary();
    
    // Test 4: Test Authentication
    cout << "\n--- Testing Authentication ---" << endl;
    if (customer1.verifyPin("1234")) {
        cout << "PIN verification successful!" << endl;
    } else {
        cout << "PIN verification failed!" << endl;
    }
    
    // Test 5: Account Operations
    cout << "\n--- Account Operations ---" << endl;
    Account* foundAccount = customer1.findAccount("SAV001");
    if (foundAccount) {
        foundAccount->deposit(200.0);
        foundAccount->withdraw(100.0);
    }
    
    // Test 6: Financial Summary
    cout << "\n--- Financial Summary ---" << endl;
    cout << "Total Balance: $" << fixed << setprecision(2) << customer1.getTotalBalance() << endl;
    cout << "Active Accounts: " << customer1.getTotalActiveAccounts() << endl;
    
    // Test 7: Update Contact Info
    cout << "\n--- Updating Contact Info ---" << endl;
    customer1.updateContactInfo("456 Oak Ave", "555-0456", "john.doe@email.com");
    
    cout << "\n=== Testing Complete ===" << endl;
    
    return 0;
}
