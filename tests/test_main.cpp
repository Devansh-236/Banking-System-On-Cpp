// Unit and integration tests for the banking system.
// Self-contained (no external framework): build and run with `make test`.

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <vector>
#include <cmath>
#include <set>
#include "Bank.h"
#include "Login.h"
#include "FileManager.h"
#include "Utilities.h"

using namespace std;
namespace fs = std::filesystem;

// =============================================================================
// MINIMAL TEST FRAMEWORK
// =============================================================================

static int checksRun = 0;
static int checksFailed = 0;
static vector<pair<string, function<void()>>>& registry() {
    static vector<pair<string, function<void()>>> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(const string& name, function<void()> fn) { registry().emplace_back(name, fn); }
};

#define TEST(name)                                         \
    static void name();                                    \
    static TestRegistrar registrar_##name(#name, name);    \
    static void name()

#define CHECK(condition)                                                                    \
    do {                                                                                    \
        checksRun++;                                                                        \
        if (!(condition)) {                                                                 \
            checksFailed++;                                                                 \
            cout << "    FAILED: " << #condition << "  (" << __FILE__ << ":" << __LINE__ << ")" << endl; \
        }                                                                                   \
    } while (0)

#define CHECK_MONEY(actual, expected) CHECK(std::abs((actual) - (expected)) < 0.005)

// Unique temporary directory, removed when the test finishes
struct TempDir {
    fs::path path;
    TempDir() {
        static int counter = 0;
        path = fs::temp_directory_path() /
               ("banking_test_" + to_string(time(nullptr)) + "_" + to_string(++counter));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
    string sub(const string& name) const { return (path / name).string(); }
};

static unique_ptr<Bank> makeBank(const TempDir& dir) {
    return make_unique<Bank>("Test Bank", "TST001", dir.sub("data"), dir.sub("backup"), dir.sub("logs"));
}

// =============================================================================
// UTILITIES
// =============================================================================

TEST(sha256_matches_known_vectors) {
    CHECK(sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256("password") == "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8");
    CHECK(sha256(string(1000, 'a')) == "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
}

TEST(pin_hash_is_salted) {
    CHECK(hashPin("1234", "CUST000001") != hashPin("1234", "CUST000002"));
    CHECK(hashPin("1234", "CUST000001") == hashPin("1234", "CUST000001"));
}

TEST(currency_formatting) {
    CHECK(formatCurrency(0) == "$0.00");
    CHECK(formatCurrency(1234567.891) == "$1,234,567.89");
    CHECK(formatCurrency(-900) == "-$900.00");
    CHECK(formatCurrency(999.995) == "$1,000.00");
    CHECK(roundMoney(0.1 + 0.2) == 0.3);
}

TEST(date_validation_and_arithmetic) {
    CHECK(isValidDate("2024-02-29"));
    CHECK(!isValidDate("2023-02-29"));
    CHECK(!isValidDate("2024-13-01"));
    CHECK(!isValidDate("2024-1-01"));
    CHECK(isValidMonth("2026-10"));
    CHECK(!isValidMonth("2026-00"));
    CHECK(addDaysToDate("2024-12-31", 1) == "2025-01-01");
    CHECK(addDaysToDate("2024-03-01", -1) == "2024-02-29");
    CHECK(addDaysToDate("2025-01-01", -365) == "2024-01-02");
}

TEST(field_validation) {
    CHECK(isValidEmail("jane.doe+bank@example.co.uk"));
    CHECK(!isValidEmail("no-at-sign.com"));
    CHECK(!isValidEmail("two@@example.com"));
    CHECK(!isValidEmail("user@.com"));
    CHECK(isValidPhone("+1 (555) 012-3456"));
    CHECK(!isValidPhone("12345"));
    CHECK(!isValidPhone("555-CALL-NOW"));
    CHECK(isValidName("Mary-Jane O'Neil"));
    CHECK(!isValidName("R2D2"));
    CHECK(isValidPin("1234") && isValidPin("123456"));
    CHECK(!isValidPin("123") && !isValidPin("1234567") && !isValidPin("12a4"));
    CHECK(isValidAccountNumber("SAV00000001") && !isValidAccountNumber("XYZ00000001"));
}

TEST(record_escaping_round_trips) {
    vector<string> fields = {"plain", "with|pipe", "back\\slash", "multi\nline", "", "end\\"};
    CHECK(splitRecord(joinRecord(fields)) == fields);
    CHECK(splitRecord("a|b||c").size() == 4);
}

// =============================================================================
// ACCOUNTS, CUSTOMERS, LOANS
// =============================================================================

TEST(savings_account_rules) {
    SavingsAccount account("SAV00000001", "CUST000001", 1000.0, 0.06, 500.0, 2);
    string reason;
    CHECK(!account.canWithdraw(600.0, reason));        // would go below the minimum balance
    CHECK(reason.find("minimum") != string::npos);
    CHECK(account.withdraw(100.0));
    CHECK(account.withdraw(100.0));
    CHECK(!account.withdraw(10.0));                    // monthly limit of 2 reached
    account.resetWithdrawalCount();
    CHECK(account.withdraw(10.0));
    CHECK_MONEY(account.getBalance(), 790.0);
    CHECK_MONEY(account.calculateInterest(), 3.95);    // 790 * 6% / 12
    CHECK_MONEY(account.applyInterest(), 3.95);
    account.setActive(false);
    CHECK(!account.deposit(10.0));
    CHECK(!account.canWithdraw(1.0, reason));
}

TEST(checking_account_overdraft_and_fees) {
    CheckingAccount account("CHK00000001", "CUST000001", 100.0, 200.0, 1.50, 2);
    CHECK(account.withdraw(250.0));                    // into the overdraft
    CHECK_MONEY(account.getBalance(), -150.0);
    CHECK(!account.withdraw(60.0));                    // beyond the overdraft
    CHECK(account.deposit(10.0));
    CHECK(account.deposit(10.0));
    CHECK_MONEY(account.calculateMonthlyFee(), 1.5);   // 3 transactions (failed one not counted), 2 free
    CHECK_MONEY(account.chargeFee(), 1.5);
    CHECK_MONEY(account.getBalance(), -131.5);
    account.resetTransactionCount();
    CHECK_MONEY(account.calculateMonthlyFee(), 0.0);
}

TEST(customer_pin_and_lockout) {
    Customer customer("CUST000001", "Alice", "1 Road", "5550001111", "alice@example.com", "4321");
    CHECK(customer.verifyPin("4321"));
    CHECK(!customer.verifyPin("1234"));
    CHECK(!customer.changePin("0000", "1111"));
    CHECK(!customer.changePin("4321", "12"));
    CHECK(customer.changePin("4321", "1111"));
    CHECK(customer.verifyPin("1111"));
    CHECK(!customer.recordFailedLogin(3));
    CHECK(!customer.recordFailedLogin(3));
    CHECK(customer.recordFailedLogin(3));
    CHECK(customer.isAccountLocked());
    customer.setLocked(false);
    CHECK(!customer.isAccountLocked() && customer.getFailedLoginAttempts() == 0);
    CHECK(customer.updateContactInfo("", "", "new@example.com"));
    CHECK(customer.getAddress() == "1 Road");
    CHECK(customer.getEmail() == "new@example.com");
    CHECK(!customer.updateContactInfo("", "", "bad-email"));
}

TEST(loan_amortisation) {
    CHECK_MONEY(Loan::calculateMonthlyPayment(10000, 0.12, 12), 888.49);
    CHECK_MONEY(Loan::calculateMonthlyPayment(1200, 0.0, 12), 100.0);
    Loan loan("LN000001", "CUST000001", "SAV00000001", 1200.0, 0.0, 12);
    for (int i = 0; i < 12; i++) loan.makePayment(loan.getMonthlyPayment());
    CHECK(loan.getStatus() == LoanStatus::PAID_OFF);
    CHECK_MONEY(loan.getOutstandingBalance(), 0.0);
    CHECK_MONEY(loan.makePayment(10.0), 0.0);          // nothing more to pay
}

// =============================================================================
// BANK OPERATIONS
// =============================================================================

TEST(customer_creation_validates_input) {
    TempDir dir;
    auto bank = makeBank(dir);
    string id = bank->createCustomer("John Doe", "123 Main St", "555-012-3456", "john@email.com", "1234");
    CHECK(id == "CUST000001");
    CHECK(bank->createCustomer("Jane", "x", "5550000000", "JOHN@email.com", "1234").empty()); // duplicate email
    CHECK(bank->getLastError().find("already exists") != string::npos);
    CHECK(bank->createCustomer("Jane", "x", "5550000000", "jane@email.com", "12").empty());    // bad PIN
    CHECK(bank->createCustomer("J4ne", "x", "5550000000", "jane@email.com", "1234").empty());  // bad name
    CHECK(bank->getTotalCustomers() == 1);
    CHECK(bank->authenticateCustomer("cust000001", "1234"));   // IDs are case-insensitive
    CHECK(!bank->authenticateCustomer(id, "9999"));
}

TEST(opening_deposit_is_not_doubled) {
    TempDir dir;
    auto bank = makeBank(dir);
    string id = bank->createCustomer("John Doe", "1 Road", "5550001111", "john@email.com", "1234");
    string savings = bank->createSavingsAccount(id, 1000.0);
    string checking = bank->createCheckingAccount(id, 250.0);
    CHECK(savings == "SAV00000001" && checking == "CHK00000001");
    CHECK_MONEY(bank->getAccountBalance(savings), 1000.0);
    CHECK_MONEY(bank->getAccountBalance(checking), 250.0);
    CHECK(bank->createSavingsAccount(id, 499.99).empty());
    CHECK(bank->createCheckingAccount("CUST999999", 500.0).empty());
    auto history = bank->getAccountTransactionHistory(savings);
    CHECK(history.size() == 1 && history[0].getTransactionType() == TransactionType::ACCOUNT_OPENING);
    CHECK(bank->validateSystemIntegrity());
}

TEST(deposit_withdraw_transfer_and_limits) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string b = bank->createCustomer("Bob", "2 Road", "5550002222", "b@example.com", "2222");
    string sav = bank->createSavingsAccount(a, 2000.0);
    string chk = bank->createCheckingAccount(a, 500.0, 300.0);
    string bobChk = bank->createCheckingAccount(b, 200.0);

    CHECK(bank->deposit(sav, 100.0));
    CHECK(!bank->deposit(sav, -5.0));
    CHECK(!bank->deposit("SAV99999999", 5.0));
    CHECK(bank->withdraw(chk, 700.0));                 // uses $200 of the overdraft
    CHECK_MONEY(bank->getAccountBalance(chk), -200.0);
    CHECK(!bank->withdraw(chk, 200.0));                // beyond the overdraft
    CHECK(bank->transfer(sav, bobChk, 300.0, "Rent"));
    CHECK_MONEY(bank->getAccountBalance(sav), 1800.0);
    CHECK_MONEY(bank->getAccountBalance(bobChk), 500.0);
    CHECK(!bank->transfer(sav, sav, 10.0));
    CHECK(!bank->transfer(sav, bobChk, 20000.0));      // over MAX_TRANSFER_AMOUNT
    CHECK(bank->getLastError().find("limited") != string::npos);

    // Transfer legs are linked and attributed to the right customers
    auto out = bank->getAccountTransactionHistory(sav, 1)[0];
    auto in = bank->getAccountTransactionHistory(bobChk, 1)[0];
    CHECK(out.getTransactionType() == TransactionType::TRANSFER_OUT && out.getCustomerId() == a);
    CHECK(in.getTransactionType() == TransactionType::TRANSFER_IN && in.getCustomerId() == b);
    CHECK(out.getLinkedTransactionId() == in.getTransactionId());

    // Daily cash withdrawal limit ($5,000 default)
    CHECK(bank->deposit(bobChk, 9000.0));
    CHECK(bank->withdraw(bobChk, 4000.0));
    CHECK(!bank->withdraw(bobChk, 1500.0));
    CHECK(bank->getLastError().find("Daily withdrawal limit") != string::npos);
    CHECK(bank->withdraw(bobChk, 1000.0));
    CHECK(bank->validateSystemIntegrity());
}

TEST(frozen_accounts_block_activity) {
    TempDir dir;
    auto bank = makeBank(dir);
    string id = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string sav = bank->createSavingsAccount(id, 1000.0);
    CHECK(bank->freezeAccount(sav, "test"));
    CHECK(!bank->freezeAccount(sav, "again"));
    CHECK(!bank->deposit(sav, 10.0));
    CHECK(!bank->withdraw(sav, 10.0));
    CHECK(bank->unfreezeAccount(sav));
    CHECK(bank->deposit(sav, 10.0));
    CHECK(bank->setCustomerActive(id, false));
    CHECK(!bank->findAccount(sav)->isAccountActive());
    CHECK(!bank->unfreezeAccount(sav));                // owner inactive
    CHECK(!bank->authenticateCustomer(id, "1111"));
    CHECK(bank->setCustomerActive(id, true));
    CHECK(!bank->findAccount(sav)->isAccountActive());   // reactivation does not unfreeze
    CHECK(bank->unfreezeAccount(sav));

    // A frozen account cannot be emptied by closing it, except by an admin
    CHECK(bank->freezeAccount(sav, "fraud"));
    CHECK(!bank->closeAccount(sav));
    CHECK(bank->findAccount(sav) != nullptr);
    CHECK(bank->closeAccount(sav, "", true));
}

TEST(closing_accounts_and_deleting_customers) {
    TempDir dir;
    auto bank = makeBank(dir);
    string id = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string sav = bank->createSavingsAccount(id, 1000.0);
    string chk = bank->createCheckingAccount(id, 200.0);
    string chk2 = bank->createCheckingAccount(id, 200.0);

    CHECK(bank->closeAccount(sav, chk));               // balance moved, not lost
    CHECK(bank->findAccount(sav) == nullptr);
    CHECK_MONEY(bank->getAccountBalance(chk), 1200.0);
    CHECK(bank->findCustomer(id)->getAccountCount() == 2);

    CHECK(bank->withdraw(chk2, 400.0));                // overdrawn
    CHECK(!bank->closeAccount(chk2));
    CHECK(!bank->deleteCustomer(id));
    CHECK(bank->deposit(chk2, 200.0));

    CHECK(bank->deleteCustomer(id));
    CHECK(bank->findCustomer(id) == nullptr);
    CHECK(bank->getTotalAccounts() == 0);
    // Closed account numbers are never reused
    string other = bank->createCustomer("Bob", "2 Road", "5550002222", "b@example.com", "2222");
    CHECK(bank->createSavingsAccount(other, 600.0) == "SAV00000002");
    CHECK(bank->validateSystemIntegrity());
}

TEST(ownership_transfer_and_type_conversion) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string b = bank->createCustomer("Bob", "2 Road", "5550002222", "b@example.com", "2222");
    string chk = bank->createCheckingAccount(a, 800.0);

    CHECK(bank->transferAccountOwnership(chk, b));
    CHECK(bank->customerOwnsAccount(b, chk));
    CHECK(bank->findCustomer(a)->getAccountCount() == 0);
    CHECK(bank->findCustomer(b)->getAccountCount() == 1);
    CHECK(bank->deposit(chk, 1.0));                    // account still alive after the move

    string sav = bank->changeAccountType(chk, AccountType::SAVINGS);
    CHECK(sav == "SAV00000001");
    CHECK(bank->findAccount(chk) == nullptr);
    CHECK_MONEY(bank->getAccountBalance(sav), 801.0);
    CHECK(bank->customerOwnsAccount(b, sav));
    CHECK(bank->validateSystemIntegrity());
}

TEST(conversion_keeps_monthly_fees_and_limits) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string chk = bank->createCheckingAccount(a, 1000.0);
    for (int i = 0; i < 12; i++) CHECK(bank->deposit(chk, 1.0));    // 2 billable transactions
    string sav = bank->changeAccountType(chk, AccountType::SAVINGS);
    CHECK(!sav.empty());
    CHECK_MONEY(bank->getAccountBalance(sav), 1012.0 - 5.0);        // fees settled first
    auto* savings = dynamic_cast<SavingsAccount*>(bank->findAccount(sav));
    CHECK(savings && savings->getRemainingWithdrawals() == 0);      // activity carried over
    CHECK(bank->validateSystemIntegrity());
}

TEST(reversals_restore_balances) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string sav = bank->createSavingsAccount(a, 1000.0);
    string chk = bank->createCheckingAccount(a, 500.0);

    CHECK(bank->deposit(sav, 250.0));
    string depositId = bank->getAccountTransactionHistory(sav, 1)[0].getTransactionId();
    CHECK(bank->reverseTransaction(depositId, "Bounced cheque"));
    CHECK_MONEY(bank->getAccountBalance(sav), 1000.0);
    CHECK(!bank->reverseTransaction(depositId, "twice"));
    CHECK(bank->getTransactionManager().findTransaction(depositId)->getStatus() == TransactionStatus::REVERSED);

    CHECK(bank->transfer(sav, chk, 300.0));
    string inId = bank->getAccountTransactionHistory(chk, 1)[0].getTransactionId();
    CHECK(bank->reverseTransaction(inId, "Sent by mistake"));    // either leg reverses both
    CHECK_MONEY(bank->getAccountBalance(sav), 1000.0);
    CHECK_MONEY(bank->getAccountBalance(chk), 500.0);
    CHECK_MONEY(bank->getAccountNetFlow(sav), 1000.0);
    CHECK(bank->validateSystemIntegrity());
}

TEST(month_end_processing) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string sav = bank->createSavingsAccount(a, 1200.0);
    string chk = bank->createCheckingAccount(a, 1000.0);
    for (int i = 0; i < 12; i++) CHECK(bank->deposit(chk, 1.0));   // 12 txns, 10 free

    CHECK(bank->processMonthEnd());
    CHECK_MONEY(bank->getAccountBalance(sav), 1204.0);              // 1200 * 4% / 12
    CHECK_MONEY(bank->getAccountBalance(chk), 1012.0 - 5.0);        // 2 x $2.50
    CHECK(!bank->processMonthEnd());                                // already run this month
    CHECK(bank->processMonthEnd(true));
    CHECK_MONEY(bank->getAccountBalance(chk), 1007.0);              // counters were reset
    CHECK(bank->validateSystemIntegrity());
}

TEST(loans_through_the_bank) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string b = bank->createCustomer("Bob", "2 Road", "5550002222", "b@example.com", "2222");
    string chk = bank->createCheckingAccount(a, 500.0);
    string bobChk = bank->createCheckingAccount(b, 500.0);

    CHECK(bank->applyForLoan(a, bobChk, 1000.0, 12).empty());       // not her account
    CHECK(bank->applyForLoan(a, chk, 60000.0, 12).empty());         // over the cap
    string loanId = bank->applyForLoan(a, chk, 1000.0, 12);
    CHECK(loanId == "LN000001");
    CHECK_MONEY(bank->getAccountBalance(chk), 1500.0);
    CHECK(!bank->closeAccount(chk));                                // linked to an active loan
    CHECK(!bank->deleteCustomer(a));

    CHECK(bank->makeLoanPayment(loanId, 5000.0));                   // capped at the outstanding amount
    CHECK(bank->findLoan(loanId)->getStatus() == LoanStatus::PAID_OFF);
    CHECK_MONEY(bank->getAccountBalance(chk), 500.0);
    CHECK(!bank->makeLoanPayment(loanId, 10.0));
    CHECK(bank->closeAccount(chk));
    CHECK(bank->deleteCustomer(a));                                 // paid-off loan stays as history
    CHECK(bank->validateSystemIntegrity());
}

// =============================================================================
// PERSISTENCE AND MAINTENANCE
// =============================================================================

TEST(save_and_reload_round_trip) {
    TempDir dir;
    string loanId, sav, chk, a;
    {
        auto bank = makeBank(dir);
        a = bank->createCustomer("Alice O'Neil", "1 Road | Flat 2", "5550001111", "a@example.com", "1111");
        sav = bank->createSavingsAccount(a, 1000.0);
        chk = bank->createCheckingAccount(a, 500.0, 750.0);
        CHECK(bank->deposit(sav, 123.45, "Pay|check with pipe"));
        CHECK(bank->withdraw(chk, 900.0));
        loanId = bank->applyForLoan(a, chk, 2000.0, 24);
        CHECK(bank->freezeAccount(sav, "audit"));
        CHECK(bank->saveAllData());
    }
    auto bank = makeBank(dir);
    CHECK(bank->loadAllData());
    CHECK(bank->getLastError().empty());
    CHECK(bank->getTotalCustomers() == 1 && bank->getTotalAccounts() == 2);
    Customer* alice = bank->findCustomer(a);
    CHECK(alice && alice->getAddress() == "1 Road | Flat 2");
    CHECK(alice && alice->getAccountCount() == 2);
    CHECK(bank->authenticateCustomer(a, "1111"));                   // PIN hash survived
    CHECK_MONEY(bank->getAccountBalance(sav), 1123.45);
    CHECK_MONEY(bank->getAccountBalance(chk), 1600.0);
    CHECK(!bank->findAccount(sav)->isAccountActive());
    CHECK_MONEY(dynamic_cast<CheckingAccount*>(bank->findAccount(chk))->getOverdraftLimit(), 750.0);
    Loan* loan = bank->findLoan(loanId);
    CHECK(loan && loan->isActive() && loan->getTermMonths() == 24);
    auto history = bank->getAccountTransactionHistory(sav);
    CHECK(history.size() == 2 && history[0].getDescription() == "Pay|check with pipe");
    CHECK(history[0].getTimestamp() == history[0].getTimestamp().substr(0, 19));  // timestamp kept
    CHECK(bank->validateSystemIntegrity());

    // New IDs continue after the saved ones instead of colliding
    set<string> ids;
    for (const auto& txn : bank->getTransactionManager().getAllTransactions()) ids.insert(txn.getTransactionId());
    CHECK(bank->deposit(chk, 1.0));
    string newId = bank->getAccountTransactionHistory(chk, 1)[0].getTransactionId();
    CHECK(ids.count(newId) == 0);
    CHECK(bank->createCheckingAccount(a, 100.0) == "CHK00000002");
    CHECK(bank->applyForLoan(a, chk, 100.0, 1) == "LN000002");
    string b = bank->createCustomer("Bob", "2 Road", "5550002222", "b@example.com", "2222");
    CHECK(b == "CUST000002");
}

TEST(integrity_check_detects_tampering) {
    TempDir dir;
    string sav;
    {
        auto bank = makeBank(dir);
        string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
        sav = bank->createSavingsAccount(a, 1000.0);
        CHECK(bank->saveAllData());
    }
    // Edit the balance in accounts.dat behind the bank's back
    string path = dir.sub("data") + "/accounts.dat";
    ifstream in(path);
    stringstream content;
    content << in.rdbuf();
    in.close();
    string text = content.str();
    size_t pos = text.find("1000.00");
    CHECK(pos != string::npos);
    text.replace(pos, 7, "9999.00");
    ofstream(path) << text;

    auto bank = makeBank(dir);
    bank->loadAllData();
    vector<string> issues = bank->findIntegrityIssues();
    CHECK(issues.size() == 1);
    CHECK(!issues.empty() && issues[0].find(sav) != string::npos);
}

TEST(backup_and_restore) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string sav = bank->createSavingsAccount(a, 1000.0);
    string backupName;
    CHECK(bank->backupSystemData(backupName));
    CHECK(bank->listBackups().size() == 1);
    CHECK(bank->deposit(sav, 500.0));
    CHECK(bank->restoreSystemData(backupName));
    CHECK_MONEY(bank->getAccountBalance(sav), 1000.0);
    CHECK(bank->listBackups().size() == 2);                          // safety backup taken first
    CHECK(!bank->restoreSystemData("../../etc"));
}

TEST(transaction_history_queries) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string chk = bank->createCheckingAccount(a, 100.0);
    for (int i = 1; i <= 5; i++) CHECK(bank->deposit(chk, i * 10.0));
    auto latest = bank->getAccountTransactionHistory(chk, 2);
    CHECK(latest.size() == 2);
    CHECK_MONEY(latest[0].getAmount(), 50.0);                        // newest first, not oldest
    CHECK_MONEY(latest[1].getAmount(), 40.0);
    const TransactionManager& manager = bank->getTransactionManager();
    string today = getCurrentDate();
    CHECK(manager.getAccountTransactionsByDateRange(chk, today, today).size() == 6);  // end date inclusive
    CHECK(manager.getAccountTransactionsByDateRange(chk, addDaysToDate(today, 1), "").empty());
    CHECK_MONEY(manager.getTotalDeposits(chk), 250.0);
    CHECK(manager.isValidTransactionId(latest[0].getTransactionId()));
    CHECK(!manager.isValidTransactionId("TXN2025-07-001"));

    string csv = dir.sub("statement.csv");
    CHECK(bank->exportAccountStatement(chk, csv));
    ifstream file(csv);
    int lines = 0;
    for (string line; getline(file, line);) lines++;
    CHECK(lines == 7);                                               // header + 6 rows
}

TEST(key_value_update_preserves_comments) {
    TempDir dir;
    string path = dir.sub("settings.txt");
    ofstream(path) << "# Comment\nA=1\n\n# Another\nB=2\n";
    CHECK(FileManager::updateKeyValueFile(path, "B", "3"));
    CHECK(FileManager::updateKeyValueFile(path, "C", "4"));
    map<string, string> values;
    CHECK(FileManager::loadKeyValueFile(path, values));
    CHECK(values["A"] == "1" && values["B"] == "3" && values["C"] == "4");
    ifstream in(path);
    stringstream content;
    content << in.rdbuf();
    CHECK(content.str().find("# Another") != string::npos);
    CHECK(FileManager::parseSize("10MB", 0) == 10ULL * 1024 * 1024);
    CHECK(FileManager::parseSize("junk", 7) == 7);
}

// =============================================================================
// LOGIN
// =============================================================================

TEST(customer_login_lockout_and_sessions) {
    TempDir dir;
    auto bank = makeBank(dir);
    string a = bank->createCustomer("Alice", "1 Road", "5550001111", "a@example.com", "1111");
    string chk = bank->createCheckingAccount(a, 500.0);
    LoginManager login(*bank, dir.sub("admin_settings.txt"));

    CHECK(login.loginCustomer(a, "1111") == LoginResult::SUCCESS);
    CHECK(login.isLoggedIn() && !login.isAdmin() && login.getCurrentUserId() == a);
    CHECK(bank->deposit(chk, 5.0));
    CHECK(bank->getAccountTransactionHistory(chk, 1)[0].getSessionId() == login.getSessionId());
    CHECK(login.touchSession());
    login.logout();
    CHECK(!login.isLoggedIn());

    CHECK(login.loginCustomer(a, "0000") == LoginResult::INVALID_CREDENTIALS);
    CHECK(login.loginCustomer(a, "0000") == LoginResult::INVALID_CREDENTIALS);
    CHECK(login.loginCustomer(a, "0000") == LoginResult::ACCOUNT_LOCKED);
    CHECK(login.loginCustomer(a, "1111") == LoginResult::ACCOUNT_LOCKED);   // correct PIN still refused
    CHECK(bank->unlockCustomer(a));
    CHECK(login.loginCustomer(a, "1111") == LoginResult::SUCCESS);
    CHECK(login.loginCustomer("CUST424242", "1111") == LoginResult::INVALID_CREDENTIALS);
}

TEST(admin_login_and_password_change) {
    TempDir dir;
    auto bank = makeBank(dir);
    string settings = dir.sub("admin_settings.txt");
    ofstream(settings) << "# Admin\nADMIN_USERNAME=admin\n"
                          "ADMIN_PASSWORD_HASH=5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8\n"
                          "PASSWORD_MIN_LENGTH=8\nREQUIRE_SPECIAL_CHARS=true\n";
    LoginManager login(*bank, settings);
    CHECK(login.loadAdminSettings());
    CHECK(login.loginAdmin("admin", "wrong") == LoginResult::INVALID_CREDENTIALS);
    CHECK(login.loginAdmin("admin", "password") == LoginResult::SUCCESS);
    CHECK(login.isAdmin());

    string error;
    CHECK(!login.validatePasswordStrength("short1!", error));
    CHECK(!login.validatePasswordStrength("longpassword1", error));          // no special character
    CHECK(login.validatePasswordStrength("S3cure!pass", error));
    CHECK(!login.changeAdminPassword("wrong", "S3cure!pass"));
    CHECK(login.changeAdminPassword("password", "S3cure!pass"));

    string broken = dir.sub("broken_settings.txt");
    ofstream(broken) << "ID,Timestamp,Account\nnot a settings file\n";
    LoginManager disabled(*bank, broken);
    disabled.loadAdminSettings();
    CHECK(disabled.loginAdmin("admin", "password") != LoginResult::SUCCESS);   // no default fallback

    LoginManager fresh(*bank, settings);
    CHECK(fresh.loadAdminSettings());
    CHECK(fresh.loginAdmin("admin", "password") == LoginResult::INVALID_CREDENTIALS);
    CHECK(fresh.loginAdmin("admin", "S3cure!pass") == LoginResult::SUCCESS);
}

// =============================================================================
// RUNNER
// =============================================================================

int main() {
    int testsFailed = 0;
    for (const auto& [name, test] : registry()) {
        int before = checksFailed;
        test();
        bool passed = checksFailed == before;
        if (!passed) testsFailed++;
        cout << (passed ? "[PASS] " : "[FAIL] ") << name << endl;
    }
    cout << "\n" << registry().size() - testsFailed << "/" << registry().size() << " tests passed, "
         << checksRun - checksFailed << "/" << checksRun << " checks passed." << endl;
    return testsFailed == 0 ? 0 : 1;
}
