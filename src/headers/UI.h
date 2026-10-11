#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include <stdexcept>
#include "Bank.h"
#include "Login.h"

using namespace std;

// Thrown when standard input is closed (Ctrl+D / end of piped input)
class InputClosedException : public runtime_error {
public:
    InputClosedException() : runtime_error("Input closed") {}
};

// Menu-driven console interface. All prompting and printing lives here; the
// Bank and LoginManager only return results and error messages.
class UI {
private:
    Bank& bank;
    LoginManager& login;

    // Input helpers
    string readLine(const string& prompt) const;
    string readNonEmpty(const string& prompt) const;
    string readSecret(const string& prompt) const;          // No echo on a terminal
    int readInt(const string& prompt, int minValue, int maxValue) const;
    bool readAmount(const string& prompt, double& amount, bool allowZero = false) const;  // false = cancelled
    bool readRate(const string& prompt, double& rate) const;       // Percent in, fraction out
    bool confirm(const string& prompt) const;
    int menuChoice(int maxOption) const;

    // Output helpers
    void printHeader(const string& title) const;
    void printSuccess(const string& message) const;
    void printError(const string& message) const;
    void printInfo(const string& message) const;

    // Shared helpers
    string selectOwnAccount(const string& prompt, bool activeOnly);
    bool sessionValid();

    // Main menu actions
    void customerLogin();
    void registerCustomer();
    void adminLogin();

    // Customer menu
    void customerMenu();
    void showAccountSummary();
    void showAccountDetails();
    void customerDeposit();
    void customerWithdraw();
    void customerTransfer();
    void showTransactionHistory();
    void showStatementByDate();
    void exportStatement();
    void openNewAccount();
    void customerCloseAccount();
    void loanMenu();
    void applyForLoan();
    void makeLoanPayment();
    void updateContactDetails();
    void changePin();

    // Admin menu
    void adminMenu();
    void adminCustomerDetails();
    void adminAccountHistory();
    void adminLargeTransactions();
    void adminDailyReport();
    void adminMonthlyReport();
    void adminFreezeAccount();
    void adminUnfreezeAccount();
    void adminToggleCustomer();
    void adminUnlockOrResetPin();
    void adminCloseAccount();
    void adminDeleteCustomer();
    void adminTransferOwnership();
    void adminConvertAccount();
    void adminSetOverdraft();
    void adminReverseTransaction();
    void adminMonthEnd();
    void adminConfiguration();
    void adminBackup();
    void adminRestore();
    void adminChangePassword();

public:
    UI(Bank& bankRef, LoginManager& loginRef);
    void run();
};

#endif // UI_H
