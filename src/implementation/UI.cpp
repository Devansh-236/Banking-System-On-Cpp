#include "UI.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <algorithm>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// =============================================================================
// TERMINAL HELPERS
// =============================================================================

static bool stdoutIsTerminal() {
#ifndef _WIN32
    static const bool terminal = isatty(STDOUT_FILENO);
    return terminal;
#else
    return false;
#endif
}

static string colour(const string& code, const string& text) {
    return stdoutIsTerminal() ? "\033[" + code + "m" + text + "\033[0m" : text;
}

// =============================================================================
// CONSTRUCTOR AND MAIN LOOP
// =============================================================================

UI::UI(Bank& bankRef, LoginManager& loginRef) : bank(bankRef), login(loginRef) {}

void UI::run() {
    try {
        while (true) {
            printHeader(bank.getBankName() + " - Main Menu");
            cout << "  1. Customer Login" << endl;
            cout << "  2. Register as a New Customer" << endl;
            cout << "  3. Administrator Login" << endl;
            cout << "  0. Exit" << endl;
            switch (menuChoice(3)) {
            case 1: customerLogin(); break;
            case 2: registerCustomer(); break;
            case 3: adminLogin(); break;
            case 0:
                cout << "\nThank you for banking with " << bank.getBankName() << ". Goodbye!" << endl;
                return;
            }
        }
    } catch (const InputClosedException&) {
        login.logout();
        cout << "\nInput closed. Exiting." << endl;
    }
}

// =============================================================================
// INPUT HELPERS
// =============================================================================

string UI::readLine(const string& prompt) const {
    cout << prompt;
    cout.flush();
    string line;
    if (!getline(cin, line)) {
        throw InputClosedException();
    }
    return trim(line);
}

string UI::readNonEmpty(const string& prompt) const {
    while (true) {
        string value = readLine(prompt);
        if (!value.empty()) return value;
        printError("This field cannot be empty.");
    }
}

string UI::readSecret(const string& prompt) const {
#ifndef _WIN32
    if (isatty(STDIN_FILENO)) {
        cout << prompt;
        cout.flush();
        termios oldSettings{};
        tcgetattr(STDIN_FILENO, &oldSettings);
        termios hidden = oldSettings;
        hidden.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &hidden);
        string line;
        bool ok = static_cast<bool>(getline(cin, line));
        tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
        cout << endl;
        if (!ok) throw InputClosedException();
        return trim(line);
    }
#endif
    return readLine(prompt);
}

int UI::readInt(const string& prompt, int minValue, int maxValue) const {
    while (true) {
        string value = readLine(prompt);
        try {
            size_t used = 0;
            int number = stoi(value, &used);
            if (used == value.size() && number >= minValue && number <= maxValue) {
                return number;
            }
        } catch (...) {
        }
        printError("Please enter a whole number from " + to_string(minValue) + " to " + to_string(maxValue) + ".");
    }
}

bool UI::readAmount(const string& prompt, double& amount, bool allowZero) const {
    while (true) {
        string value = readLine(prompt + " (blank to cancel): $");
        if (value.empty()) return false;
        value.erase(remove(value.begin(), value.end(), ','), value.end());
        size_t dot = value.find('.');
        bool digitsOnly = !value.empty() && value.find_first_not_of("0123456789.") == string::npos &&
                          value.find('.', dot == string::npos ? 0 : dot + 1) == string::npos;
        bool centsOk = dot == string::npos || value.size() - dot - 1 <= 2;
        try {
            size_t used = 0;
            double number = (digitsOnly && centsOk) ? stod(value, &used) : -1.0;
            if (used == value.size() && std::isfinite(number) && (number > 0 || (allowZero && number == 0)) &&
                number <= 1e12) {
                amount = roundMoney(number);
                return true;
            }
        } catch (...) {
        }
        printError(string("Enter a ") + (allowZero ? "non-negative" : "positive") +
                   " amount with at most two decimal places, e.g. 250 or 99.95.");
    }
}

bool UI::readRate(const string& prompt, double& rate) const {
    while (true) {
        string value = readLine(prompt + " in % (blank to cancel): ");
        if (value.empty()) return false;
        if (!value.empty() && value.back() == '%') value.pop_back();
        try {
            size_t used = 0;
            double percent = stod(value, &used);
            if (used == value.size() && std::isfinite(percent) && percent >= 0) {
                rate = percent / 100.0;
                return true;
            }
        } catch (...) {
        }
        printError("Enter a percentage such as 4 or 3.75.");
    }
}

bool UI::confirm(const string& prompt) const {
    while (true) {
        string answer = toUpper(readLine(prompt + " (y/n): "));
        if (answer == "Y" || answer == "YES") return true;
        if (answer == "N" || answer == "NO") return false;
        printError("Please answer y or n.");
    }
}

int UI::menuChoice(int maxOption) const {
    cout << endl;
    return readInt("Select an option: ", 0, maxOption);
}

// =============================================================================
// OUTPUT HELPERS
// =============================================================================

void UI::printHeader(const string& title) const {
    cout << "\n" << colour("1;36", string(60, '=')) << endl;
    cout << colour("1", "  " + title) << endl;
    cout << colour("1;36", string(60, '=')) << endl;
}

void UI::printSuccess(const string& message) const {
    cout << colour("32", "[OK] ") << message << endl;
}

void UI::printError(const string& message) const {
    cout << colour("31", "[ERROR] ") << message << endl;
}

void UI::printInfo(const string& message) const {
    cout << colour("33", "[INFO] ") << message << endl;
}

// =============================================================================
// SHARED HELPERS
// =============================================================================

// Let the logged-in customer pick one of their accounts; "" if cancelled/none
string UI::selectOwnAccount(const string& prompt, bool activeOnly) {
    vector<Account*> accounts = bank.getCustomerAccounts(login.getCurrentUserId());
    vector<Account*> choices;
    for (Account* account : accounts) {
        if (!activeOnly || account->isAccountActive()) choices.push_back(account);
    }
    if (choices.empty()) {
        printInfo(accounts.empty() ? "You don't have any accounts yet. Open one from the customer menu."
                                   : "You have no active accounts.");
        return "";
    }
    cout << "\n" << prompt << endl;
    for (size_t i = 0; i < choices.size(); i++) {
        cout << "  " << i + 1 << ". " << left << setw(13) << choices[i]->getAccountNumber()
             << setw(10) << choices[i]->getAccountTypeString()
             << right << setw(16) << formatCurrency(choices[i]->getBalance())
             << (choices[i]->isAccountActive() ? "" : "  [FROZEN]") << endl;
    }
    cout << "  0. Cancel" << endl;
    int choice = readInt("Account: ", 0, static_cast<int>(choices.size()));
    return choice == 0 ? "" : choices[choice - 1]->getAccountNumber();
}

bool UI::sessionValid() {
    if (login.touchSession()) return true;
    printInfo(login.getLastMessage());
    return false;
}

// =============================================================================
// MAIN MENU ACTIONS
// =============================================================================

void UI::customerLogin() {
    printHeader("Customer Login");
    string customerId = readNonEmpty("Customer ID (e.g. CUST000001): ");
    string pin = readSecret("PIN: ");
    if (login.loginCustomer(customerId, pin) != LoginResult::SUCCESS) {
        printError(login.getLastMessage());
        return;
    }
    printSuccess(login.getLastMessage());
    customerMenu();
}

void UI::registerCustomer() {
    printHeader("New Customer Registration");
    cout << "Please enter your details. All fields are required." << endl;
    string name = readNonEmpty("Full name      : ");
    string address = readNonEmpty("Address        : ");
    string phone = readNonEmpty("Phone          : ");
    string email = readNonEmpty("Email          : ");
    string pin = readSecret("Choose a PIN (4-6 digits): ");
    string pinAgain = readSecret("Confirm PIN    : ");
    if (pin != pinAgain) {
        printError("The PINs do not match. Registration cancelled.");
        return;
    }
    string customerId = bank.createCustomer(name, address, phone, email, pin);
    if (customerId.empty()) {
        printError(bank.getLastError());
        return;
    }
    printSuccess("Registration complete!");
    cout << "\n  Your Customer ID is: " << colour("1", customerId) << endl;
    cout << "  Keep it safe - you need it with your PIN to log in." << endl;
    cout << "  Log in to open your first savings or checking account." << endl;
}

void UI::adminLogin() {
    printHeader("Administrator Login");
    string username = readNonEmpty("Username: ");
    string password = readSecret("Password: ");
    if (login.loginAdmin(username, password) != LoginResult::SUCCESS) {
        printError(login.getLastMessage());
        return;
    }
    printSuccess(login.getLastMessage());
    adminMenu();
}

// =============================================================================
// CUSTOMER MENU
// =============================================================================

void UI::customerMenu() {
    while (login.isLoggedIn() && !login.isAdmin()) {
        Customer* customer = bank.findCustomer(login.getCurrentUserId());
        if (!customer) {
            login.logout();
            return;
        }
        printHeader("Welcome, " + customer->getName() + " (" + customer->getCustomerId() + ")");
        cout << "  Total balance: " << formatCurrency(customer->getTotalBalance()) << endl << endl;
        cout << "  1. Account Summary              8. Export Statement (CSV)" << endl;
        cout << "  2. Account Details              9. Open a New Account" << endl;
        cout << "  3. Deposit                     10. Close an Account" << endl;
        cout << "  4. Withdraw                    11. Loans" << endl;
        cout << "  5. Transfer Funds              12. Update Contact Details" << endl;
        cout << "  6. Transaction History         13. Change PIN" << endl;
        cout << "  7. Statement by Date Range      0. Logout" << endl;
        int choice = menuChoice(13);
        if (choice == 0) {
            login.logout();
            printSuccess("You have been logged out.");
            return;
        }
        if (!sessionValid()) return;
        switch (choice) {
        case 1: showAccountSummary(); break;
        case 2: showAccountDetails(); break;
        case 3: customerDeposit(); break;
        case 4: customerWithdraw(); break;
        case 5: customerTransfer(); break;
        case 6: showTransactionHistory(); break;
        case 7: showStatementByDate(); break;
        case 8: exportStatement(); break;
        case 9: openNewAccount(); break;
        case 10: customerCloseAccount(); break;
        case 11: loanMenu(); break;
        case 12: updateContactDetails(); break;
        case 13: changePin(); break;
        }
    }
}

void UI::showAccountSummary() {
    printHeader("Account Summary");
    Customer* customer = bank.findCustomer(login.getCurrentUserId());
    customer->displayAccountSummary();
    double owed = bank.getCustomerOutstandingLoans(customer->getCustomerId());
    if (owed > 0) {
        cout << "Loans Outstanding : " << formatCurrency(owed) << endl;
    }
}

void UI::showAccountDetails() {
    printHeader("Account Details");
    bank.findCustomer(login.getCurrentUserId())->displayDetailedAccountInfo();
}

void UI::customerDeposit() {
    printHeader("Deposit");
    string accountNumber = selectOwnAccount("Deposit into which account?", true);
    if (accountNumber.empty()) return;
    double amount;
    if (!readAmount("Amount to deposit", amount)) return;
    string description = readLine("Description (optional): ");
    if (!sessionValid()) return;
    if (bank.deposit(accountNumber, amount, description.empty() ? "Cash deposit" : description)) {
        printSuccess("Deposited " + formatCurrency(amount) + ". New balance: " +
                     formatCurrency(bank.getAccountBalance(accountNumber)));
    } else {
        printError(bank.getLastError());
    }
}

void UI::customerWithdraw() {
    printHeader("Withdraw");
    string accountNumber = selectOwnAccount("Withdraw from which account?", true);
    if (accountNumber.empty()) return;
    double amount;
    if (!readAmount("Amount to withdraw", amount)) return;
    if (!sessionValid()) return;
    if (bank.withdraw(accountNumber, amount, "Cash withdrawal")) {
        printSuccess("Withdrew " + formatCurrency(amount) + ". New balance: " +
                     formatCurrency(bank.getAccountBalance(accountNumber)));
    } else {
        printError(bank.getLastError());
    }
}

void UI::customerTransfer() {
    printHeader("Transfer Funds");
    string fromAccount = selectOwnAccount("Transfer from which account?", true);
    if (fromAccount.empty()) return;

    cout << "\n  1. To another of my accounts" << endl;
    cout << "  2. To another customer's account" << endl;
    cout << "  0. Cancel" << endl;
    int choice = readInt("Destination: ", 0, 2);
    if (choice == 0) return;

    string toAccount;
    if (choice == 1) {
        toAccount = selectOwnAccount("Transfer to which account?", true);
        if (toAccount.empty()) return;
    } else {
        toAccount = toUpper(readNonEmpty("Recipient account number: "));
        if (!bank.findAccount(toAccount)) {
            printError("Account " + toAccount + " not found.");
            return;
        }
    }
    if (toAccount == fromAccount) {
        printError("Cannot transfer to the same account.");
        return;
    }
    double amount;
    if (!readAmount("Amount to transfer", amount)) return;
    string description = readLine("Reference (optional): ");
    if (!confirm("Transfer " + formatCurrency(amount) + " from " + fromAccount + " to " + toAccount + "?")) {
        printInfo("Transfer cancelled.");
        return;
    }
    if (!sessionValid()) return;
    if (bank.transfer(fromAccount, toAccount, amount, description.empty() ? "Funds transfer" : description)) {
        printSuccess("Transferred " + formatCurrency(amount) + ". " + fromAccount + " balance: " +
                     formatCurrency(bank.getAccountBalance(fromAccount)));
    } else {
        printError(bank.getLastError());
    }
}

void UI::showTransactionHistory() {
    printHeader("Transaction History");
    string accountNumber = selectOwnAccount("Show history for which account?", false);
    if (accountNumber.empty()) return;
    int count = readInt("How many recent transactions (1-500)? ", 1, 500);
    cout << endl;
    bank.getTransactionManager().displayTransactionHistory(accountNumber, count);
}

void UI::showStatementByDate() {
    printHeader("Statement by Date Range");
    string accountNumber = selectOwnAccount("Statement for which account?", false);
    if (accountNumber.empty()) return;
    string startDate = readLine("Start date (YYYY-MM-DD, blank = first transaction): ");
    string endDate = readLine("End date   (YYYY-MM-DD, blank = today): ");
    if ((!startDate.empty() && !isValidDate(startDate)) || (!endDate.empty() && !isValidDate(endDate))) {
        printError("Dates must be valid and in YYYY-MM-DD format.");
        return;
    }
    if (!startDate.empty() && !endDate.empty() && startDate > endDate) {
        printError("Start date must be on or before the end date.");
        return;
    }
    const TransactionManager& manager = bank.getTransactionManager();
    vector<Transaction> transactions = manager.getAccountTransactionsByDateRange(accountNumber, startDate, endDate);
    cout << "\nStatement for " << accountNumber << " from " << (startDate.empty() ? "the beginning" : startDate)
         << " to " << (endDate.empty() ? "today" : endDate) << endl;
    cout << string(110, '=') << endl;
    TransactionManager::displayTransactionTable(transactions);
    cout << string(110, '-') << endl;
    cout << "Credits: " << formatCurrency(manager.getTotalDeposits(accountNumber, startDate, endDate))
         << "   Debits: " << formatCurrency(manager.getTotalWithdrawals(accountNumber, startDate, endDate))
         << "   Net: " << formatCurrency(manager.getNetFlow(accountNumber, startDate, endDate)) << endl;
}

void UI::exportStatement() {
    printHeader("Export Statement");
    string accountNumber = selectOwnAccount("Export which account?", false);
    if (accountNumber.empty()) return;
    // Statements may only be written as plain file names inside <data>/exports
    filesystem::path exportDir = filesystem::path(bank.getFileManager().getDataDirectory()) / "exports";
    string defaultName = "statement_" + accountNumber + "_" + getCurrentDate() + ".csv";
    string name = readLine("File name [" + defaultName + "]: ");
    if (name.empty()) name = defaultName;
    if (name.find_first_of("/\\:") != string::npos || name.find("..") != string::npos || name[0] == '.') {
        printError("Please enter a plain file name (no folders); statements are saved in " + exportDir.string() + ".");
        return;
    }
    if (filesystem::path(name).extension() != ".csv") name += ".csv";
    string path = (exportDir / name).string();
    if (filesystem::exists(path) && !confirm(path + " already exists. Overwrite it?")) return;
    if (!sessionValid()) return;
    if (bank.exportAccountStatement(accountNumber, path)) {
        printSuccess("Statement exported to " + path);
    } else {
        printError(bank.getLastError());
    }
}

void UI::openNewAccount() {
    printHeader("Open a New Account");
    cout << "  1. Savings  - " << "minimum opening deposit " << formatCurrency(bank.getMinSavingsBalance())
         << ", earns monthly interest" << endl;
    cout << "  2. Checking - " << "minimum opening deposit " << formatCurrency(bank.getMinCheckingBalance())
         << ", overdraft up to " << formatCurrency(bank.getDefaultOverdraftLimit()) << endl;
    cout << "  0. Cancel" << endl;
    int choice = readInt("Account type: ", 0, 2);
    if (choice == 0) return;
    double deposit;
    if (!readAmount("Opening deposit", deposit)) return;
    if (!sessionValid()) return;
    string accountNumber = choice == 1 ? bank.createSavingsAccount(login.getCurrentUserId(), deposit)
                                       : bank.createCheckingAccount(login.getCurrentUserId(), deposit);
    if (accountNumber.empty()) {
        printError(bank.getLastError());
        return;
    }
    printSuccess(string(choice == 1 ? "Savings" : "Checking") + " account " + accountNumber +
                 " opened with " + formatCurrency(deposit) + ".");
}

void UI::customerCloseAccount() {
    printHeader("Close an Account");
    string accountNumber = selectOwnAccount("Close which account?", true);
    if (accountNumber.empty()) return;
    double balance = bank.getAccountBalance(accountNumber);
    string transferTo;
    if (balance > 0) {
        cout << "This account holds " << formatCurrency(balance) << "." << endl;
        cout << "  1. Move the balance to another of my accounts" << endl;
        cout << "  2. Withdraw the balance as cash" << endl;
        cout << "  0. Cancel" << endl;
        int choice = readInt("Choice: ", 0, 2);
        if (choice == 0) return;
        if (choice == 1) {
            transferTo = selectOwnAccount("Move the balance to which account?", true);
            if (transferTo.empty()) return;
        }
    }
    if (!confirm("Permanently close account " + accountNumber + "?")) {
        printInfo("Account not closed.");
        return;
    }
    if (!sessionValid()) return;
    if (bank.closeAccount(accountNumber, transferTo)) {
        printSuccess("Account " + accountNumber + " closed." +
                     (balance > 0 ? (transferTo.empty() ? " " + formatCurrency(balance) + " paid out."
                                                        : " Balance moved to " + transferTo + ".")
                                  : ""));
    } else {
        printError(bank.getLastError());
    }
}

void UI::loanMenu() {
    while (sessionValid()) {
        printHeader("Loans");
        bank.displayCustomerLoans(login.getCurrentUserId());
        cout << "\n  1. Loan Details" << endl;
        cout << "  2. Apply for a Loan" << endl;
        cout << "  3. Make a Loan Payment" << endl;
        cout << "  0. Back" << endl;
        int choice = menuChoice(3);
        if (choice == 0 || !sessionValid()) return;
        if (choice == 1) {
            string loanId = toUpper(readNonEmpty("Loan ID: "));
            Loan* loan = bank.findLoan(loanId);
            if (!loan || loan->getCustomerId() != login.getCurrentUserId()) {
                printError("Loan " + loanId + " not found.");
            } else {
                cout << endl;
                loan->displayLoanInfo();
            }
        } else if (choice == 2) {
            applyForLoan();
        } else {
            makeLoanPayment();
        }
    }
}

void UI::applyForLoan() {
    string customerId = login.getCurrentUserId();
    double available = max(0.0, bank.getMaxLoanAmount() - bank.getCustomerOutstandingLoans(customerId));
    cout << "\nCurrent loan rate: " << fixed << setprecision(2) << bank.getLoanInterestRate() * 100
         << "% p.a.   You can borrow up to " << formatCurrency(available) << "." << endl;
    double amount;
    if (!readAmount("Loan amount", amount)) return;
    int months = readInt("Term in months (1-360): ", 1, 360);
    string accountNumber = selectOwnAccount("Disburse into (and repay from) which account?", true);
    if (accountNumber.empty()) return;

    double emi = Loan::calculateMonthlyPayment(amount, bank.getLoanInterestRate(), months);
    cout << "\n  Monthly payment : " << formatCurrency(emi) << endl;
    cout << "  Total repayable : " << formatCurrency(emi * months) << endl;
    cout << "  Total interest  : " << formatCurrency(emi * months - amount) << endl;
    if (!confirm("Accept this loan?")) {
        printInfo("Loan application cancelled.");
        return;
    }
    if (!sessionValid()) return;
    string loanId = bank.applyForLoan(customerId, accountNumber, amount, months);
    if (loanId.empty()) {
        printError(bank.getLastError());
        return;
    }
    printSuccess("Loan " + loanId + " approved. " + formatCurrency(amount) + " has been credited to " + accountNumber + ".");
}

void UI::makeLoanPayment() {
    vector<Loan*> activeLoans;
    for (Loan* loan : bank.getCustomerLoans(login.getCurrentUserId())) {
        if (loan->isActive()) activeLoans.push_back(loan);
    }
    if (activeLoans.empty()) {
        printInfo("You have no active loans.");
        return;
    }
    cout << "\nWhich loan?" << endl;
    for (size_t i = 0; i < activeLoans.size(); i++) {
        cout << "  " << i + 1 << ". " << activeLoans[i]->getLoanId() << "  outstanding "
             << formatCurrency(activeLoans[i]->getOutstandingBalance()) << ", monthly payment "
             << formatCurrency(activeLoans[i]->getMonthlyPayment()) << ", paid from "
             << activeLoans[i]->getLinkedAccountNumber() << endl;
    }
    cout << "  0. Cancel" << endl;
    int choice = readInt("Loan: ", 0, static_cast<int>(activeLoans.size()));
    if (choice == 0) return;
    Loan* loan = activeLoans[choice - 1];
    double amount;
    if (!readAmount("Payment amount", amount)) return;
    if (!sessionValid()) return;
    if (bank.makeLoanPayment(loan->getLoanId(), amount)) {
        if (loan->isActive()) {
            printSuccess("Payment received. Outstanding balance: " + formatCurrency(loan->getOutstandingBalance()));
        } else {
            printSuccess("Loan " + loan->getLoanId() + " is now fully paid off. Congratulations!");
        }
    } else {
        printError(bank.getLastError());
    }
}

void UI::updateContactDetails() {
    printHeader("Update Contact Details");
    Customer* customer = bank.findCustomer(login.getCurrentUserId());
    cout << "Leave a field blank to keep the current value." << endl;
    string address = readLine("Address [" + customer->getAddress() + "]: ");
    string phone = readLine("Phone   [" + customer->getPhoneNumber() + "]: ");
    string email = readLine("Email   [" + customer->getEmail() + "]: ");
    if (address.empty() && phone.empty() && email.empty()) {
        printInfo("Nothing changed.");
        return;
    }
    if (!sessionValid()) return;
    if (bank.updateCustomerInfo(customer->getCustomerId(), address, phone, email)) {
        printSuccess("Contact details updated.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::changePin() {
    printHeader("Change PIN");
    string oldPin = readSecret("Current PIN: ");
    string newPin = readSecret("New PIN (4-6 digits): ");
    string confirmPin = readSecret("Confirm new PIN: ");
    if (newPin != confirmPin) {
        printError("The new PINs do not match.");
        return;
    }
    if (!sessionValid()) return;
    if (bank.changeCustomerPin(login.getCurrentUserId(), oldPin, newPin)) {
        printSuccess("PIN changed.");
    } else {
        printError(bank.getLastError());
    }
}

// =============================================================================
// ADMIN MENU
// =============================================================================

void UI::adminMenu() {
    while (login.isAdmin()) {
        printHeader("Administrator Menu");
        cout << "  Reports                          Customers & Accounts" << endl;
        cout << "   1. System Statistics             12. Freeze Account" << endl;
        cout << "   2. List Customers                13. Unfreeze Account" << endl;
        cout << "   3. List Accounts                 14. Activate/Deactivate Customer" << endl;
        cout << "   4. Customer Details              15. Unlock Customer / Reset PIN" << endl;
        cout << "   5. Account Transaction History   16. Close Account" << endl;
        cout << "   6. Large Transactions            17. Delete Customer" << endl;
        cout << "   7. Frozen Accounts               18. Transfer Account Ownership" << endl;
        cout << "   8. Multi-Account Customers       19. Convert Account Type" << endl;
        cout << "   9. Daily Report                  20. Set Overdraft Limit" << endl;
        cout << "  10. Monthly Report                21. Reverse a Transaction" << endl;
        cout << "  11. All Loans" << endl;
        cout << "\n  Operations" << endl;
        cout << "  22. Month-End Processing          25. Backup Data" << endl;
        cout << "  23. System Configuration          26. Restore from Backup" << endl;
        cout << "  24. System Maintenance            27. Change Admin Password" << endl;
        cout << "\n   0. Logout" << endl;
        int choice = menuChoice(27);
        if (choice == 0) {
            login.logout();
            printSuccess("Administrator logged out.");
            return;
        }
        if (!sessionValid()) return;
        cout << endl;
        switch (choice) {
        case 1: bank.displaySystemStatistics(); break;
        case 2: bank.displayAllCustomers(); break;
        case 3: bank.displayAllAccounts(); break;
        case 4: adminCustomerDetails(); break;
        case 5: adminAccountHistory(); break;
        case 6: adminLargeTransactions(); break;
        case 7: bank.displayInactiveAccounts(); break;
        case 8: bank.displayCustomersWithMultipleAccounts(); break;
        case 9: adminDailyReport(); break;
        case 10: adminMonthlyReport(); break;
        case 11: bank.displayAllLoans(); break;
        case 12: adminFreezeAccount(); break;
        case 13: adminUnfreezeAccount(); break;
        case 14: adminToggleCustomer(); break;
        case 15: adminUnlockOrResetPin(); break;
        case 16: adminCloseAccount(); break;
        case 17: adminDeleteCustomer(); break;
        case 18: adminTransferOwnership(); break;
        case 19: adminConvertAccount(); break;
        case 20: adminSetOverdraft(); break;
        case 21: adminReverseTransaction(); break;
        case 22: adminMonthEnd(); break;
        case 23: adminConfiguration(); break;
        case 24: bank.performSystemMaintenance(); break;
        case 25: adminBackup(); break;
        case 26: adminRestore(); break;
        case 27: adminChangePassword(); break;
        }
    }
}

void UI::adminCustomerDetails() {
    string customerId = toUpper(readNonEmpty("Customer ID: "));
    cout << endl;
    bank.displayCustomerDetails(customerId);
}

void UI::adminAccountHistory() {
    string accountNumber = toUpper(readNonEmpty("Account number: "));
    if (!bank.findAccount(accountNumber) && bank.getAccountTransactionHistory(accountNumber, 1).empty()) {
        printError("Account " + accountNumber + " not found.");
        return;
    }
    cout << endl;
    bank.getTransactionManager().displayTransactionHistory(accountNumber, 100);
}

void UI::adminLargeTransactions() {
    double threshold;
    if (!readAmount("Show transactions of at least", threshold)) return;
    cout << endl;
    bank.displayLargeTransactions(threshold);
}

void UI::adminDailyReport() {
    string date = readLine("Date (YYYY-MM-DD, blank = today): ");
    cout << endl;
    bank.generateDailyReport(date.empty() ? getCurrentDate() : date);
}

void UI::adminMonthlyReport() {
    string month = readLine("Month (YYYY-MM, blank = this month): ");
    cout << endl;
    bank.generateMonthlyReport(month.empty() ? getCurrentMonth() : month);
}

void UI::adminFreezeAccount() {
    string accountNumber = toUpper(readNonEmpty("Account number to freeze: "));
    string reason = readNonEmpty("Reason: ");
    if (!sessionValid()) return;
    if (bank.freezeAccount(accountNumber, reason)) {
        printSuccess("Account " + accountNumber + " frozen.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminUnfreezeAccount() {
    string accountNumber = toUpper(readNonEmpty("Account number to unfreeze: "));
    if (!sessionValid()) return;
    if (bank.unfreezeAccount(accountNumber)) {
        printSuccess("Account " + accountNumber + " unfrozen.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminToggleCustomer() {
    string customerId = toUpper(readNonEmpty("Customer ID: "));
    Customer* customer = bank.findCustomer(customerId);
    if (!customer) {
        printError(bank.getLastError());
        return;
    }
    bool activate = !customer->isCustomerActive();
    string action = activate ? "Reactivate" : "Deactivate";
    cout << customer->getName() << " is currently " << (activate ? "inactive" : "active") << "." << endl;
    if (!activate) cout << "Deactivating also freezes all of the customer's accounts." << endl;
    else cout << "Accounts stay frozen after reactivation; unfreeze them individually." << endl;
    if (!confirm(action + " " + customerId + "?")) return;
    if (!sessionValid()) return;
    if (bank.setCustomerActive(customerId, activate)) {
        printSuccess("Customer " + customerId + (activate ? " reactivated." : " deactivated."));
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminUnlockOrResetPin() {
    string customerId = toUpper(readNonEmpty("Customer ID: "));
    Customer* customer = bank.findCustomer(customerId);
    if (!customer) {
        printError(bank.getLastError());
        return;
    }
    cout << customer->getName() << (customer->isAccountLocked() ? " is LOCKED" : " is not locked")
         << " (" << customer->getFailedLoginAttempts() << " failed attempt(s))." << endl;
    cout << "  1. Unlock only" << endl;
    cout << "  2. Reset PIN (also unlocks)" << endl;
    cout << "  0. Cancel" << endl;
    int choice = readInt("Choice: ", 0, 2);
    if (choice == 1) {
        if (!sessionValid()) return;
        if (bank.unlockCustomer(customerId)) printSuccess("Customer " + customerId + " unlocked.");
        else printError(bank.getLastError());
    } else if (choice == 2) {
        string pin = readSecret("New PIN (4-6 digits): ");
        string again = readSecret("Confirm new PIN: ");
        if (pin != again) {
            printError("The PINs do not match.");
            return;
        }
        if (!sessionValid()) return;
        if (bank.resetCustomerPin(customerId, pin)) printSuccess("PIN reset for " + customerId + ".");
        else printError(bank.getLastError());
    }
}

void UI::adminCloseAccount() {
    string accountNumber = toUpper(readNonEmpty("Account number to close: "));
    Account* account = bank.findAccount(accountNumber);
    if (!account) {
        printError("Account " + accountNumber + " not found.");
        return;
    }
    string transferTo;
    if (account->getBalance() > 0) {
        cout << "Balance: " << formatCurrency(account->getBalance()) << endl;
        transferTo = toUpper(readLine("Move balance to account (blank = pay out as cash): "));
    }
    if (!confirm("Close account " + accountNumber + "?")) return;
    if (!sessionValid()) return;
    if (bank.closeAccount(accountNumber, transferTo, true)) {
        printSuccess("Account " + accountNumber + " closed.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminDeleteCustomer() {
    string customerId = toUpper(readNonEmpty("Customer ID to delete: "));
    Customer* customer = bank.findCustomer(customerId);
    if (!customer) {
        printError(bank.getLastError());
        return;
    }
    cout << customer->getName() << " has " << customer->getAccountCount() << " account(s) holding "
         << formatCurrency(customer->getTotalBalance()) << "." << endl;
    cout << "All accounts will be closed and positive balances paid out." << endl;
    string typed = toUpper(readLine("Type the customer ID again to confirm: "));
    if (typed != customerId) {
        printInfo("Deletion cancelled.");
        return;
    }
    if (!sessionValid()) return;
    if (bank.deleteCustomer(customerId)) {
        printSuccess("Customer " + customerId + " deleted.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminTransferOwnership() {
    string accountNumber = toUpper(readNonEmpty("Account number: "));
    string newOwner = toUpper(readNonEmpty("New owner customer ID: "));
    if (!confirm("Transfer " + accountNumber + " to " + newOwner + "?")) return;
    if (!sessionValid()) return;
    if (bank.transferAccountOwnership(accountNumber, newOwner)) {
        printSuccess("Account " + accountNumber + " now belongs to " + newOwner + ".");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminConvertAccount() {
    string accountNumber = toUpper(readNonEmpty("Account number to convert: "));
    Account* account = bank.findAccount(accountNumber);
    if (!account) {
        printError("Account " + accountNumber + " not found.");
        return;
    }
    AccountType target = account->getAccountType() == AccountType::SAVINGS ? AccountType::CHECKING : AccountType::SAVINGS;
    if (!confirm("Convert " + accountNumber + " to a " + accountTypeToString(target) +
                 " account? It will get a new account number.")) return;
    if (!sessionValid()) return;
    string newNumber = bank.changeAccountType(accountNumber, target);
    if (newNumber.empty()) {
        printError(bank.getLastError());
    } else {
        printSuccess("Converted. New account number: " + newNumber);
    }
}

void UI::adminSetOverdraft() {
    string accountNumber = toUpper(readNonEmpty("Checking account number: "));
    double limit;
    if (!readAmount("New overdraft limit", limit, true)) return;
    if (!sessionValid()) return;
    if (bank.setAccountOverdraftLimit(accountNumber, limit)) {
        printSuccess("Overdraft limit for " + accountNumber + " set to " + formatCurrency(limit) + ".");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminReverseTransaction() {
    string transactionId = toUpper(readNonEmpty("Transaction ID: "));
    const Transaction* txn = bank.getTransactionManager().findTransaction(transactionId);
    if (!txn) {
        printError("Transaction " + transactionId + " not found.");
        return;
    }
    cout << endl;
    txn->displayTransaction();
    cout << endl;
    string reason = readNonEmpty("Reason for reversal: ");
    if (!confirm("Reverse this transaction?")) return;
    if (!sessionValid()) return;
    if (bank.reverseTransaction(transactionId, reason)) {
        printSuccess("Transaction " + transactionId + " reversed.");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminMonthEnd() {
    cout << "Month-end processing credits savings interest, charges checking fees," << endl;
    cout << "resets monthly counters and accrues loan interest." << endl;
    if (!confirm("Run month-end processing now?") || !sessionValid()) return;
    cout << endl;
    if (bank.processMonthEnd(false)) {
        printSuccess("Month-end processing complete.");
        return;
    }
    printInfo(bank.getLastError());
    if (confirm("Run it again anyway?")) {
        cout << endl;
        if (bank.processMonthEnd(true)) printSuccess("Month-end processing complete.");
        else printError(bank.getLastError());
    }
}

void UI::adminConfiguration() {
    while (sessionValid()) {
        cout << endl;
        bank.displaySystemConfiguration();
        cout << "\n  1. Set Minimum Balances" << endl;
        cout << "  2. Set Savings Interest Rate" << endl;
        cout << "  3. Set Loan Interest Rate" << endl;
        cout << "  4. Set Default Overdraft Limit" << endl;
        cout << "  5. Set Withdrawal/Transfer Limits" << endl;
        cout << "  0. Back" << endl;
        int choice = menuChoice(5);
        if (choice == 0 || !sessionValid()) return;
        bool ok = false;
        if (choice == 1) {
            double savingsMin, checkingMin;
            if (!readAmount("Minimum savings balance", savingsMin, true) ||
                !readAmount("Minimum checking balance", checkingMin, true)) continue;
            if (!sessionValid()) return;
            ok = bank.setMinimumBalances(savingsMin, checkingMin);
        } else if (choice == 2) {
            double rate;
            if (!readRate("Savings interest rate", rate)) continue;
            if (!sessionValid()) return;
            ok = bank.setSavingsInterestRate(rate);
        } else if (choice == 3) {
            double rate;
            if (!readRate("Loan interest rate (new loans)", rate)) continue;
            if (!sessionValid()) return;
            ok = bank.setLoanInterestRate(rate);
        } else if (choice == 4) {
            double limit;
            if (!readAmount("Default overdraft limit", limit, true)) continue;
            if (!sessionValid()) return;
            ok = bank.setDefaultOverdraftLimit(limit);
        } else {
            double daily, transferLimit;
            if (!readAmount("Daily withdrawal limit", daily) || !readAmount("Maximum single transfer", transferLimit)) continue;
            if (!sessionValid()) return;
            ok = bank.setTransactionLimits(daily, transferLimit);
        }
        if (ok) printSuccess("Configuration updated.");
        else printError(bank.getLastError());
    }
}

void UI::adminBackup() {
    string backupName;
    if (bank.backupSystemData(backupName)) {
        printSuccess("Backup created: " + bank.getFileManager().getBackupDirectory() + "/" + backupName);
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminRestore() {
    vector<string> backups = bank.listBackups();
    if (backups.empty()) {
        printInfo("No backups available.");
        return;
    }
    cout << "Available backups (newest first):" << endl;
    size_t shown = min<size_t>(backups.size(), 15);
    for (size_t i = 0; i < shown; i++) {
        cout << "  " << i + 1 << ". " << backups[i] << endl;
    }
    cout << "  0. Cancel" << endl;
    int choice = readInt("Backup: ", 0, static_cast<int>(shown));
    if (choice == 0) return;
    cout << "Current data will be replaced (a safety backup of it is taken first)." << endl;
    if (!confirm("Restore " + backups[choice - 1] + "?")) return;
    if (!sessionValid()) return;
    if (bank.restoreSystemData(backups[choice - 1])) {
        printSuccess("Data restored from " + backups[choice - 1] + ".");
    } else {
        printError(bank.getLastError());
    }
}

void UI::adminChangePassword() {
    string current = readSecret("Current password: ");
    string newPassword = readSecret("New password: ");
    string again = readSecret("Confirm new password: ");
    if (newPassword != again) {
        printError("The new passwords do not match.");
        return;
    }
    if (!sessionValid()) return;
    if (login.changeAdminPassword(current, newPassword)) {
        printSuccess(login.getLastMessage());
    } else {
        printError(login.getLastMessage());
    }
}
