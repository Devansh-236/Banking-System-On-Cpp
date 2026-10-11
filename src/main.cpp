#include <iostream>
#include <iomanip>
#include <filesystem>
#include <map>
#include "Bank.h"
#include "Login.h"
#include "UI.h"

using namespace std;
namespace fs = std::filesystem;

struct Options {
    string dataDir = "data";
    string backupDir = "backup";
    string logDir = "logs";
    string configDir = "config";
    bool demo = false;
};

static void printUsage(const char* program) {
    cout << "Usage: " << program << " [options]\n\n"
         << "Options:\n"
         << "  --data-dir DIR     Directory for data files      (default: data)\n"
         << "  --backup-dir DIR   Directory for backups         (default: backup)\n"
         << "  --log-dir DIR      Directory for log files       (default: logs)\n"
         << "  --config-dir DIR   Directory with bank_config.txt and admin_settings.txt (default: config)\n"
         << "  --demo             Run a scripted walkthrough in a temporary directory and exit\n"
         << "  -h, --help         Show this help\n";
}

// Returns false if the program should exit (help shown or bad arguments)
static bool parseArguments(int argc, char* argv[], Options& options, int& exitCode) {
    map<string, string*> valueOptions = {{"--data-dir", &options.dataDir},
                                         {"--backup-dir", &options.backupDir},
                                         {"--log-dir", &options.logDir},
                                         {"--config-dir", &options.configDir}};
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exitCode = 0;
            return false;
        }
        if (arg == "--demo") {
            options.demo = true;
            continue;
        }
        auto it = valueOptions.find(arg);
        if (it != valueOptions.end() && i + 1 < argc) {
            *it->second = argv[++i];
            continue;
        }
        cerr << "Unknown or incomplete option: " << arg << "\n\n";
        printUsage(argv[0]);
        exitCode = 1;
        return false;
    }
    return true;
}

static void step(const string& title) {
    cout << "\n--- " << title << " " << string(max<int>(0, 60 - static_cast<int>(title.size())), '-') << endl;
}

static void report(const Bank& bank, bool ok, const string& success) {
    if (ok) cout << "[OK] " << success << endl;
    else cout << "[REJECTED] " << bank.getLastError() << endl;
}

// Scripted walkthrough of the main features (runs against a throwaway data directory)
static int runDemo(const Options& options) {
    fs::path demoRoot = fs::temp_directory_path() / ("banking_demo_" + to_string(time(nullptr)));
    Bank bank("National Banking Corp", "NBC001", (demoRoot / "data").string(),
              (demoRoot / "backup").string(), (demoRoot / "logs").string());
    bank.loadConfiguration((fs::path(options.configDir) / "bank_config.txt").string());
    cout << "=== Banking System Demo (data in " << demoRoot.string() << ") ===" << endl;

    step("Customers");
    string john = bank.createCustomer("John Doe", "123 Main St", "555-0123-456", "john@email.com", "1234");
    string jane = bank.createCustomer("Jane Smith", "456 Oak Ave", "555-0456-789", "jane@email.com", "5678");
    cout << "Created " << john << " (John Doe) and " << jane << " (Jane Smith)" << endl;
    report(bank, !bank.createCustomer("Bad Email", "1 Road", "5550000000", "not-an-email", "1111").empty(), "");

    step("Accounts");
    string savings1 = bank.createSavingsAccount(john, 1000.0);
    string checking1 = bank.createCheckingAccount(john, 500.0, 1000.0);
    string savings2 = bank.createSavingsAccount(jane, 1500.0);
    cout << "John: " << savings1 << " (savings), " << checking1 << " (checking, $1,000 overdraft)" << endl;
    cout << "Jane: " << savings2 << " (savings)" << endl;
    report(bank, !bank.createSavingsAccount(jane, 100.0).empty(), "");

    step("Banking operations");
    report(bank, bank.deposit(savings1, 500.0, "Salary deposit"), "Deposited $500.00 into " + savings1);
    report(bank, bank.withdraw(checking1, 200.0, "ATM withdrawal"), "Withdrew $200.00 from " + checking1);
    report(bank, bank.transfer(savings1, checking1, 300.0, "Internal transfer"), "Transferred $300.00 " + savings1 + " -> " + checking1);
    report(bank, bank.transfer(savings1, savings2, 50.0, "Gift for Jane"), "Transferred $50.00 to Jane");
    report(bank, bank.withdraw(savings1, 1000.0), "");
    report(bank, bank.withdraw(checking1, 1500.0, "Overdraft use"), "Withdrew $1,500.00 using the overdraft");
    cout << "Balances: " << savings1 << " " << formatCurrency(bank.getAccountBalance(savings1)) << ", "
         << checking1 << " " << formatCurrency(bank.getAccountBalance(checking1)) << ", "
         << savings2 << " " << formatCurrency(bank.getAccountBalance(savings2)) << endl;

    step("Authentication");
    report(bank, bank.authenticateCustomer(john, "1234"), john + " authenticated with the correct PIN");
    report(bank, bank.authenticateCustomer(john, "9999"), "");

    step("Loans");
    string loanId = bank.applyForLoan(jane, savings2, 5000.0, 12);
    report(bank, !loanId.empty(), "Loan " + loanId + " of $5,000.00 over 12 months approved");
    if (Loan* loan = bank.findLoan(loanId)) {
        cout << "Monthly payment: " << formatCurrency(loan->getMonthlyPayment()) << endl;
        report(bank, bank.makeLoanPayment(loanId, loan->getMonthlyPayment()), "First payment made");
    }

    step("Month-end processing");
    bank.processMonthEnd();

    step("Account services");
    report(bank, bank.freezeAccount(checking1, "Suspicious activity"), checking1 + " frozen");
    report(bank, bank.deposit(checking1, 10.0), "");
    report(bank, bank.unfreezeAccount(checking1), checking1 + " unfrozen");

    step("Transaction history for " + savings1);
    TransactionManager::displayTransactionTable(bank.getAccountTransactionHistory(savings1));

    step("Reports");
    bank.displaySystemStatistics();
    cout << endl;
    bank.displayAllCustomers();
    cout << endl;
    bank.displayAllAccounts();
    cout << endl;
    bank.displayAllLoans();
    cout << endl;
    bank.generateDailyReport(getCurrentDate());

    step("Persistence");
    report(bank, bank.saveAllData(), "All data saved");
    Bank reloaded("National Banking Corp", "NBC001", (demoRoot / "data").string(),
                  (demoRoot / "backup").string(), (demoRoot / "logs").string());
    reloaded.loadAllData();
    cout << "Reloaded: " << reloaded.getTotalCustomers() << " customers, " << reloaded.getTotalAccounts()
         << " accounts, " << reloaded.getTotalTransactions() << " transactions, total balance "
         << formatCurrency(reloaded.getTotalSystemBalance()) << endl;
    report(reloaded, reloaded.validateSystemIntegrity(), "Integrity check passed after reload");

    std::error_code ignored;
    fs::remove_all(demoRoot, ignored);
    cout << "\n=== Demo complete ===" << endl;
    return 0;
}

int main(int argc, char* argv[]) {
    Options options;
    int exitCode = 0;
    if (!parseArguments(argc, argv, options, exitCode)) {
        return exitCode;
    }
    if (options.demo) {
        return runDemo(options);
    }

    string bankConfigPath = (fs::path(options.configDir) / "bank_config.txt").string();
    string adminConfigPath = (fs::path(options.configDir) / "admin_settings.txt").string();

    map<string, string> bankSettings;
    FileManager::loadKeyValueFile(bankConfigPath, bankSettings);
    string bankName = bankSettings.count("BANK_NAME") ? bankSettings["BANK_NAME"] : "National Banking Corp";
    string bankCode = bankSettings.count("BANK_CODE") ? bankSettings["BANK_CODE"] : "NBC001";

    Bank bank(bankName, bankCode, options.dataDir, options.backupDir, options.logDir);
    if (!bank.getFileManager().ensureDirectories()) {
        cerr << "Cannot create data directories (" << options.dataDir << ", " << options.backupDir << ", "
             << options.logDir << ")." << endl;
        return 1;
    }
    if (!bank.loadConfiguration(bankConfigPath)) {
        cout << "[INFO] " << bank.getLastError() << endl;
    }

    bank.loadAllData();
    if (!bank.getLastError().empty()) {
        cout << "[WARNING] " << bank.getLastError() << endl;
    }

    LoginManager login(bank, adminConfigPath);
    if (!login.loadAdminSettings()) {
        cout << "[INFO] " << login.getLastMessage() << endl;
    }
    bank.getFileManager().setMaxLogSize(login.getMaxLogSize());
    bank.setLogRetentionDays(login.getLogRetentionDays());

    if (!bank.validateSystemIntegrity()) {
        cout << "[WARNING] Data integrity issues detected. An administrator should run System Maintenance." << endl;
    }

    // Scheduled backup on start-up when the last one is older than BACKUP_INTERVAL hours
    if (login.isAutoBackupEnabled() && bank.getTotalCustomers() > 0 &&
        bank.getFileManager().isBackupDue(bank.getBackupInterval())) {
        string backupName;
        if (bank.backupSystemData(backupName)) {
            cout << "[INFO] Automatic backup created: " << backupName << endl;
        }
    }

    bank.getFileManager().logSystem("Application started");
    bank.setAutoSave(true);

    UI ui(bank, login);
    ui.run();

    if (!bank.saveAllData()) {
        cerr << "[ERROR] " << bank.getLastError() << endl;
        return 1;
    }
    bank.getFileManager().logSystem("Application stopped");
    return 0;
}
