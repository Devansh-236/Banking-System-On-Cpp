#include "Bank.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <set>
#include <cmath>

using namespace std;

// =============================================================================
// FORMATTING HELPERS (for data files)
// =============================================================================

static string moneyField(double value) {
    stringstream ss;
    ss << fixed << setprecision(2) << value;
    return ss.str();
}

static string rateField(double value) {
    stringstream ss;
    ss << fixed << setprecision(6) << value;
    return ss.str();
}

static string boolField(bool value) {
    return value ? "1" : "0";
}

// Numeric part of an ID such as CUST000012 / SAV00000003 / LN000004 (0 if none)
static int idNumber(const string& id, size_t prefixLength) {
    if (id.size() <= prefixLength) return 0;
    return parseInt(id.substr(prefixLength), 0);
}

static string percent(double rate) {
    stringstream ss;
    ss << fixed << setprecision(2) << rate * 100 << "%";
    return ss.str();
}

// =============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// =============================================================================

Bank::Bank(string name, string code, string dataDir, string backupDir, string logDir)
    : fileManager(dataDir, backupDir, logDir)
{
    bankName = name;
    bankCode = code;
    configFilePath = "";
    autoSave = false;
    nextCustomerNumber = 1;
    nextSavingsAccountNumber = 1;
    nextCheckingAccountNumber = 1;
    nextLoanNumber = 1;
    minSavingsBalance = 500.0;
    minCheckingBalance = 100.0;
    defaultOverdraftLimit = 500.0;
    savingsInterestRate = 0.04;
    loanInterestRate = 0.08;
    maxDailyWithdrawal = 5000.0;
    maxTransferAmount = 10000.0;
    checkingTransactionFee = 2.50;
    maxLoanAmount = 50000.0;
    maxSavingsWithdrawalsPerMonth = 6;
    freeCheckingTransactions = 10;
    maxActiveLoansPerCustomer = 3;
    maxLoginAttempts = 3;
    sessionTimeoutSeconds = 1800;
    backupIntervalHours = 24;
    logRetentionDays = 30;
    lastMonthEndProcessed = "";
    totalDeposits = 0.0;
    totalWithdrawals = 0.0;
    totalCustomersCreated = 0;
    totalAccountsCreated = 0;
    transactionManager = make_unique<TransactionManager>(fileManager.getDataFilePath("transactions.dat"));
}

Bank::~Bank() = default;

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

bool Bank::fail(const string &message) const
{
    lastError = message;
    return false;
}

string Bank::failId(const string &message) const
{
    lastError = message;
    return "";
}

string Bank::getLastError() const
{
    return lastError;
}

void Bank::persist()
{
    if (autoSave && !saveAllData())
    {
        fileManager.logError("Auto-save failed: " + lastError);
    }
}

string Bank::ownerOf(const string &accountNumber) const
{
    auto it = accountCustomerMap.find(accountNumber);
    return it != accountCustomerMap.end() ? it->second : "";
}

Account *Bank::requireAccount(const string &accountNumber, bool mustBeActive)
{
    Account *account = findAccount(accountNumber);
    if (!account)
    {
        lastError = "Account " + accountNumber + " not found.";
        return nullptr;
    }
    if (mustBeActive && !account->isAccountActive())
    {
        lastError = "Account " + accountNumber + " is frozen/inactive.";
        return nullptr;
    }
    return account;
}

vector<Loan *> Bank::activeLoansForAccount(const string &accountNumber) const
{
    vector<Loan *> result;
    for (const auto &pair : loans)
    {
        if (pair.second->isActive() && pair.second->getLinkedAccountNumber() == accountNumber)
        {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

// Admin corrections may dip below a savings minimum, but never below zero
// (or past a checking overdraft limit)
double Bank::lowestBalanceForAdminDebit(const Account *account) const
{
    return min(0.0, account->getLowestAllowedBalance());
}

void Bank::registerAccount(unique_ptr<Account> account, Customer *owner)
{
    string accountNumber = account->getAccountNumber();
    Account *raw = account.get();
    accounts[accountNumber] = std::move(account);
    accountCustomerMap[accountNumber] = raw->getCustomerId();
    if (owner)
    {
        owner->addAccount(raw);
    }
}

void Bank::removeAccountRecord(const string &accountNumber)
{
    auto ownerIt = customers.find(ownerOf(accountNumber));
    if (ownerIt != customers.end())
    {
        ownerIt->second->removeAccount(accountNumber);
    }
    accountCustomerMap.erase(accountNumber);
    accounts.erase(accountNumber);
}

// Make sure generated IDs never collide with loaded data
void Bank::syncCountersWithData()
{
    for (const auto &pair : customers)
        nextCustomerNumber = max(nextCustomerNumber, idNumber(pair.first, 4) + 1);
    for (const auto &pair : accounts)
    {
        if (pair.first.compare(0, 3, "SAV") == 0)
            nextSavingsAccountNumber = max(nextSavingsAccountNumber, idNumber(pair.first, 3) + 1);
        else if (pair.first.compare(0, 3, "CHK") == 0)
            nextCheckingAccountNumber = max(nextCheckingAccountNumber, idNumber(pair.first, 3) + 1);
    }
    // Closed accounts still appear in the transaction log; never reuse their numbers
    for (const Transaction &txn : transactionManager->getAllTransactions())
    {
        const string &number = txn.getAccountNumber();
        if (number.compare(0, 3, "SAV") == 0)
            nextSavingsAccountNumber = max(nextSavingsAccountNumber, idNumber(number, 3) + 1);
        else if (number.compare(0, 3, "CHK") == 0)
            nextCheckingAccountNumber = max(nextCheckingAccountNumber, idNumber(number, 3) + 1);
    }
    for (const auto &pair : loans)
        nextLoanNumber = max(nextLoanNumber, idNumber(pair.first, 2) + 1);
}

void Bank::displayReportHeader(const string &title) const
{
    cout << string(70, '=') << endl;
    cout << "  " << bankName << " (" << bankCode << ")" << endl;
    cout << "  " << title << endl;
    cout << "  Generated: " << getCurrentDateTime() << endl;
    cout << string(70, '=') << endl;
}

// =============================================================================
// CUSTOMER MANAGEMENT
// =============================================================================

string Bank::createCustomer(string name, string address, string phone, string email, string pin)
{
    name = trim(name);
    address = trim(address);
    phone = trim(phone);
    email = trim(email);
    if (!isValidName(name))
        return failId("Invalid name. Use letters, spaces, dots, hyphens or apostrophes (max 60 characters).");
    if (address.empty())
        return failId("Address cannot be empty.");
    if (!isValidPhone(phone))
        return failId("Invalid phone number. Use 7-15 digits (spaces, dashes, brackets and a leading + are allowed).");
    if (!isValidEmail(email))
        return failId("Invalid email address.");
    if (!isValidPin(pin))
        return failId("PIN must be 4 to 6 digits.");
    if (findCustomerByEmail(email))
        return failId("A customer with this email address already exists.");

    string customerId = generateCustomerId();
    try
    {
        customers[customerId] = make_unique<Customer>(customerId, name, address, phone, email, pin);
    }
    catch (const invalid_argument &e)
    {
        return failId(e.what());
    }
    totalCustomersCreated++;
    fileManager.logSystem("Customer created: " + customerId + " (" + name + ")");
    persist();
    return customerId;
}

bool Bank::deleteCustomer(string customerId)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    for (Loan *loan : getCustomerLoans(customerId))
    {
        if (loan->isActive())
            return fail("Customer has an active loan (" + loan->getLoanId() + ") with " +
                        formatCurrency(loan->getOutstandingBalance()) + " outstanding.");
    }
    vector<Account *> customerAccounts = customer->getAllAccounts();
    for (Account *account : customerAccounts)
    {
        if (account->getBalance() < 0)
            return fail("Account " + account->getAccountNumber() + " is overdrawn (" +
                        formatCurrency(account->getBalance()) + "). Settle it before deleting the customer.");
    }

    double paidOut = 0.0;
    for (Account *account : customerAccounts)
    {
        paidOut += account->getBalance();
        string accountNumber = account->getAccountNumber();
        if (!closeAccount(accountNumber, "", true))
            return false;
    }
    customers.erase(customerId);
    fileManager.logSystem("Customer deleted: " + customerId + ", balances paid out: " + formatCurrency(paidOut));
    persist();
    return true;
}

Customer *Bank::findCustomer(string customerId) const
{
    auto it = customers.find(toUpper(trim(customerId)));
    if (it != customers.end())
    {
        return it->second.get();
    }
    lastError = "Customer " + customerId + " not found.";
    return nullptr;
}

Customer *Bank::findCustomerByEmail(string email) const
{
    string wanted = toUpper(trim(email));
    for (const auto &pair : customers)
    {
        if (toUpper(pair.second->getEmail()) == wanted)
            return pair.second.get();
    }
    return nullptr;
}

bool Bank::authenticateCustomer(string customerId, string pin) const
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return fail("Invalid customer ID or PIN.");
    if (customer->isAccountLocked())
        return fail("This customer profile is locked after too many failed attempts. Please contact the bank.");
    if (!customer->isCustomerActive())
        return fail("This customer profile is inactive. Please contact the bank.");
    if (!customer->verifyPin(pin))
        return fail("Invalid customer ID or PIN.");
    return true;
}

vector<Customer *> Bank::getAllCustomers() const
{
    vector<Customer *> customerList;
    for (const auto &pair : customers)
    {
        customerList.push_back(pair.second.get());
    }
    return customerList;
}

bool Bank::updateCustomerInfo(string customerId, string newAddress, string newPhone, string newEmail)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    newEmail = trim(newEmail);
    if (!newEmail.empty())
    {
        Customer *existing = findCustomerByEmail(newEmail);
        if (existing && existing != customer)
            return fail("Another customer already uses this email address.");
    }
    if (!customer->updateContactInfo(newAddress, newPhone, newEmail))
        return fail("Invalid phone number or email address.");
    fileManager.logSystem("Contact details updated for " + customer->getCustomerId());
    persist();
    return true;
}

bool Bank::changeCustomerPin(string customerId, string oldPin, string newPin)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    if (!customer->verifyPin(oldPin))
        return fail("Current PIN is incorrect.");
    if (oldPin == newPin)
        return fail("New PIN must be different from the current PIN.");
    if (!customer->changePin(oldPin, newPin))
        return fail("New PIN must be 4 to 6 digits.");
    fileManager.logSystem("PIN changed for " + customer->getCustomerId());
    persist();
    return true;
}

bool Bank::resetCustomerPin(string customerId, string newPin)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    if (!customer->resetPin(newPin))
        return fail("New PIN must be 4 to 6 digits.");
    customer->setLocked(false);
    fileManager.logSystem("PIN reset by admin for " + customer->getCustomerId());
    persist();
    return true;
}

bool Bank::setCustomerActive(string customerId, bool active)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    if (customer->isCustomerActive() == active)
        return fail("Customer " + customer->getCustomerId() + " is already " + (active ? "active." : "inactive."));
    // Deactivation freezes every account; reactivation deliberately does not
    // unfreeze them, since some may have been frozen for other reasons
    customer->setActive(active);
    fileManager.logSystem("Customer " + customer->getCustomerId() + (active ? " reactivated" : " deactivated"));
    persist();
    return true;
}

bool Bank::unlockCustomer(string customerId)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return false;
    if (!customer->isAccountLocked())
        return fail("Customer " + customer->getCustomerId() + " is not locked.");
    customer->setLocked(false);
    fileManager.logSystem("Customer unlocked: " + customer->getCustomerId());
    persist();
    return true;
}

// =============================================================================
// ACCOUNT MANAGEMENT
// =============================================================================

string Bank::createSavingsAccount(string customerId, double initialDeposit)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return "";
    if (!customer->isCustomerActive())
        return failId("Customer " + customer->getCustomerId() + " is inactive.");
    if (!isValidAmount(initialDeposit))
        return failId("Initial deposit must be a positive amount.");
    initialDeposit = roundMoney(initialDeposit);
    if (initialDeposit < minSavingsBalance)
        return failId("Savings accounts need an initial deposit of at least " + formatCurrency(minSavingsBalance) + ".");

    string accountNumber = generateSavingsAccountNumber();
    registerAccount(make_unique<SavingsAccount>(accountNumber, customer->getCustomerId(), initialDeposit,
                                                savingsInterestRate, minSavingsBalance, maxSavingsWithdrawalsPerMonth),
                    customer);
    transactionManager->processTransaction(TransactionType::ACCOUNT_OPENING, accountNumber, initialDeposit,
                                           "Account opening deposit", 0.0, initialDeposit, customer->getCustomerId());
    totalAccountsCreated++;
    totalDeposits += initialDeposit;
    fileManager.logTransaction("Savings account " + accountNumber + " opened for " + customer->getCustomerId() +
                               " with " + formatCurrency(initialDeposit));
    persist();
    return accountNumber;
}

string Bank::createCheckingAccount(string customerId, double initialDeposit, double overdraftLimit)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return "";
    if (!customer->isCustomerActive())
        return failId("Customer " + customer->getCustomerId() + " is inactive.");
    if (!isValidAmount(initialDeposit))
        return failId("Initial deposit must be a positive amount.");
    initialDeposit = roundMoney(initialDeposit);
    if (initialDeposit < minCheckingBalance)
        return failId("Checking accounts need an initial deposit of at least " + formatCurrency(minCheckingBalance) + ".");
    if (overdraftLimit < 0)
        overdraftLimit = defaultOverdraftLimit;
    if (overdraftLimit > maxTransferAmount)
        return failId("Overdraft limit cannot exceed " + formatCurrency(maxTransferAmount) + ".");

    string accountNumber = generateCheckingAccountNumber();
    registerAccount(make_unique<CheckingAccount>(accountNumber, customer->getCustomerId(), initialDeposit,
                                                 roundMoney(overdraftLimit), checkingTransactionFee,
                                                 freeCheckingTransactions),
                    customer);
    transactionManager->processTransaction(TransactionType::ACCOUNT_OPENING, accountNumber, initialDeposit,
                                           "Account opening deposit", 0.0, initialDeposit, customer->getCustomerId());
    totalAccountsCreated++;
    totalDeposits += initialDeposit;
    fileManager.logTransaction("Checking account " + accountNumber + " opened for " + customer->getCustomerId() +
                               " with " + formatCurrency(initialDeposit));
    persist();
    return accountNumber;
}

// Close an account. A positive balance is moved to transferToAccount if given,
// otherwise paid out in cash. Overdrawn accounts and loan-linked accounts cannot close.
bool Bank::closeAccount(string accountNumber, string transferToAccount, bool adminOverride)
{
    Account *account = requireAccount(accountNumber, !adminOverride);
    if (!account)
        return false;
    vector<Loan *> linkedLoans = activeLoansForAccount(accountNumber);
    if (!linkedLoans.empty())
        return fail("Account " + accountNumber + " is the repayment account for active loan " +
                    linkedLoans.front()->getLoanId() + ".");
    double balance = account->getBalance();
    if (balance < 0)
        return fail("Account " + accountNumber + " is overdrawn (" + formatCurrency(balance) +
                    "). Deposit funds before closing it.");

    string customerId = ownerOf(accountNumber);
    if (balance > 0 && !transferToAccount.empty())
    {
        Account *target = requireAccount(transferToAccount, true);
        if (!target)
            return false;
        if (target == account)
            return fail("Cannot transfer the closing balance to the account being closed.");
        double targetBefore = target->getBalance();
        account->updateBalance(0.0);
        target->updateBalance(targetBefore + balance);
        transactionManager->processTransfer(accountNumber, transferToAccount, balance, "Closing balance transfer",
                                            balance, 0.0, targetBefore, target->getBalance(),
                                            customerId, ownerOf(transferToAccount));
        transactionManager->processTransaction(TransactionType::ACCOUNT_CLOSING, accountNumber, 0.0,
                                               "Account closed", 0.0, 0.0, customerId);
    }
    else
    {
        account->updateBalance(0.0);
        transactionManager->processTransaction(TransactionType::ACCOUNT_CLOSING, accountNumber, balance,
                                               balance > 0 ? "Account closed - balance paid out" : "Account closed",
                                               balance, 0.0, customerId);
        if (balance > 0)
            totalWithdrawals += balance;
    }
    removeAccountRecord(accountNumber);
    fileManager.logTransaction("Account " + accountNumber + " closed (balance " + formatCurrency(balance) + ")");
    persist();
    return true;
}

Account *Bank::findAccount(string accountNumber) const
{
    auto it = accounts.find(toUpper(trim(accountNumber)));
    return it != accounts.end() ? it->second.get() : nullptr;
}

vector<Account *> Bank::getCustomerAccounts(string customerId) const
{
    Customer *customer = findCustomer(customerId);
    return customer ? customer->getAllAccounts() : vector<Account *>();
}

bool Bank::customerOwnsAccount(string customerId, string accountNumber) const
{
    Account *account = findAccount(accountNumber);
    return account && account->getCustomerId() == toUpper(trim(customerId));
}

bool Bank::transferAccountOwnership(string accountNumber, string newCustomerId)
{
    Account *account = requireAccount(accountNumber, false);
    if (!account)
        return false;
    Customer *newCustomer = findCustomer(newCustomerId);
    if (!newCustomer)
        return false;
    if (!newCustomer->isCustomerActive())
        return fail("Customer " + newCustomer->getCustomerId() + " is inactive.");
    string oldCustomerId = ownerOf(account->getAccountNumber());
    if (oldCustomerId == newCustomer->getCustomerId())
        return fail("Account already belongs to " + oldCustomerId + ".");
    if (!activeLoansForAccount(account->getAccountNumber()).empty())
        return fail("Account is linked to an active loan and cannot change owner.");

    auto oldIt = customers.find(oldCustomerId);
    if (oldIt != customers.end())
        oldIt->second->removeAccount(account->getAccountNumber());
    account->setCustomerId(newCustomer->getCustomerId());
    newCustomer->addAccount(account);
    accountCustomerMap[account->getAccountNumber()] = newCustomer->getCustomerId();
    fileManager.logSystem("Account " + account->getAccountNumber() + " ownership transferred from " +
                          oldCustomerId + " to " + newCustomer->getCustomerId());
    persist();
    return true;
}

// =============================================================================
// TRANSACTION OPERATIONS
// =============================================================================

bool Bank::deposit(string accountNumber, double amount, string description)
{
    Account *account = requireAccount(accountNumber, true);
    if (!account)
        return false;
    if (!isValidAmount(amount))
        return fail("Deposit amount must be greater than zero.");
    amount = roundMoney(amount);
    if (trim(description).empty())
        description = "Deposit";

    double balanceBefore = account->getBalance();
    if (!account->deposit(amount))
        return fail("Deposit failed for account " + account->getAccountNumber() + ".");
    string txnId = transactionManager->processDeposit(account->getAccountNumber(), amount, description,
                                                      balanceBefore, account->getBalance(),
                                                      ownerOf(account->getAccountNumber()));
    totalDeposits += amount;
    fileManager.logTransaction(txnId + " DEPOSIT " + formatCurrency(amount) + " to " + account->getAccountNumber());
    persist();
    return true;
}

bool Bank::withdraw(string accountNumber, double amount, string description)
{
    Account *account = requireAccount(accountNumber, true);
    if (!account)
        return false;
    if (!isValidAmount(amount))
        return fail("Withdrawal amount must be greater than zero.");
    amount = roundMoney(amount);
    if (trim(description).empty())
        description = "Withdrawal";

    string reason;
    if (!account->canWithdraw(amount, reason))
        return fail(reason);
    double withdrawnToday = transactionManager->getDailyWithdrawalTotal(account->getAccountNumber(), getCurrentDate());
    if (withdrawnToday + amount > maxDailyWithdrawal + 0.001)
        return fail("Daily withdrawal limit of " + formatCurrency(maxDailyWithdrawal) + " exceeded. Remaining today: " +
                    formatCurrency(max(0.0, maxDailyWithdrawal - withdrawnToday)) + ".");

    double balanceBefore = account->getBalance();
    if (!account->withdraw(amount))
        return fail("Withdrawal failed for account " + account->getAccountNumber() + ".");
    string txnId = transactionManager->processWithdrawal(account->getAccountNumber(), amount, description,
                                                         balanceBefore, account->getBalance(),
                                                         ownerOf(account->getAccountNumber()));
    totalWithdrawals += amount;
    fileManager.logTransaction(txnId + " WITHDRAWAL " + formatCurrency(amount) + " from " + account->getAccountNumber());
    persist();
    return true;
}

bool Bank::transfer(string fromAccount, string toAccount, double amount, string description)
{
    Account *fromAcc = requireAccount(fromAccount, true);
    if (!fromAcc)
        return false;
    Account *toAcc = requireAccount(toAccount, true);
    if (!toAcc)
        return false;
    if (fromAcc == toAcc)
        return fail("Cannot transfer to the same account.");
    if (!isValidAmount(amount))
        return fail("Transfer amount must be greater than zero.");
    amount = roundMoney(amount);
    if (amount > maxTransferAmount)
        return fail("Transfers are limited to " + formatCurrency(maxTransferAmount) + " each.");
    if (trim(description).empty())
        description = "Transfer";

    string reason;
    if (!fromAcc->canWithdraw(amount, reason))
        return fail(reason);

    double fromBefore = fromAcc->getBalance();
    double toBefore = toAcc->getBalance();
    if (!fromAcc->withdraw(amount))
        return fail("Transfer failed: could not debit " + fromAcc->getAccountNumber() + ".");
    if (!toAcc->deposit(amount))
    {
        fromAcc->updateBalance(fromBefore); // Roll back the debit
        return fail("Transfer failed: could not credit " + toAcc->getAccountNumber() + ".");
    }
    string txnId = transactionManager->processTransfer(fromAcc->getAccountNumber(), toAcc->getAccountNumber(), amount,
                                                       description, fromBefore, fromAcc->getBalance(),
                                                       toBefore, toAcc->getBalance(),
                                                       ownerOf(fromAcc->getAccountNumber()),
                                                       ownerOf(toAcc->getAccountNumber()));
    if (txnId.empty())
    {
        fromAcc->updateBalance(fromBefore);
        toAcc->updateBalance(toBefore);
        return fail("Transfer could not be recorded and was rolled back.");
    }
    fileManager.logTransaction(txnId + " TRANSFER " + formatCurrency(amount) + " " + fromAcc->getAccountNumber() +
                               " -> " + toAcc->getAccountNumber());
    persist();
    return true;
}

bool Bank::applyInterestToSavingsAccounts()
{
    double totalInterest = 0.0;
    int accountsWithInterest = 0;
    for (const auto &pair : accounts)
    {
        SavingsAccount *savingsAccount = dynamic_cast<SavingsAccount *>(pair.second.get());
        if (!savingsAccount)
            continue;
        double balanceBefore = savingsAccount->getBalance();
        double interest = savingsAccount->applyInterest();
        if (interest <= 0)
            continue;
        transactionManager->processInterestCredit(savingsAccount->getAccountNumber(), interest,
                                                  "Monthly interest credit", balanceBefore,
                                                  savingsAccount->getBalance(), ownerOf(pair.first));
        totalInterest += interest;
        accountsWithInterest++;
    }
    cout << "Interest credited  : " << formatCurrency(totalInterest) << " to "
         << accountsWithInterest << " savings account(s)" << endl;
    fileManager.logTransaction("Monthly interest credited: " + formatCurrency(totalInterest));
    return true;
}

void Bank::chargeMonthlyFees()
{
    double totalFeesCharged = 0.0;
    int accountsWithFees = 0;
    for (const auto &pair : accounts)
    {
        CheckingAccount *checkingAccount = dynamic_cast<CheckingAccount *>(pair.second.get());
        if (!checkingAccount)
            continue;
        double balanceBefore = checkingAccount->getBalance();
        int billable = max(0, checkingAccount->getTransactionCount() - checkingAccount->getFreeTransactions());
        double fee = checkingAccount->chargeFee();
        checkingAccount->resetTransactionCount();
        if (fee <= 0)
            continue;
        transactionManager->processFeeCharge(checkingAccount->getAccountNumber(), fee,
                                             "Monthly transaction fee (" + to_string(billable) + " billable)",
                                             balanceBefore, checkingAccount->getBalance(), ownerOf(pair.first));
        totalFeesCharged += fee;
        accountsWithFees++;
    }
    cout << "Fees charged       : " << formatCurrency(totalFeesCharged) << " to "
         << accountsWithFees << " checking account(s)" << endl;
    fileManager.logTransaction("Monthly fees charged: " + formatCurrency(totalFeesCharged));
}

double Bank::accrueLoanInterest()
{
    double totalInterest = 0.0;
    for (const auto &pair : loans)
    {
        totalInterest += pair.second->accrueMonthlyInterest();
    }
    totalInterest = roundMoney(totalInterest);
    cout << "Loan interest      : " << formatCurrency(totalInterest) << " accrued on active loans" << endl;
    fileManager.logTransaction("Loan interest accrued: " + formatCurrency(totalInterest));
    return totalInterest;
}

// Month-end cycle: savings interest, checking fees, counter resets, loan interest
bool Bank::processMonthEnd(bool force)
{
    string month = getCurrentMonth();
    if (!force && lastMonthEndProcessed == month)
        return fail("Month-end processing has already been run for " + month + ".");

    cout << "Month-end processing for " << month << endl;
    cout << string(50, '-') << endl;
    applyInterestToSavingsAccounts();
    chargeMonthlyFees();
    for (const auto &pair : accounts)
    {
        if (SavingsAccount *savings = dynamic_cast<SavingsAccount *>(pair.second.get()))
            savings->resetWithdrawalCount();
    }
    cout << "Savings withdrawal counters reset" << endl;
    accrueLoanInterest();
    lastMonthEndProcessed = month;
    fileManager.logSystem("Month-end processing completed for " + month);
    persist();
    return true;
}

// Reverse a completed transaction: moves the money back and records a
// compensating entry. Transfers reverse both legs together.
bool Bank::reverseTransaction(string transactionId, string reason)
{
    transactionId = toUpper(trim(transactionId));
    const Transaction *found = transactionManager->findTransaction(transactionId);
    if (!found)
        return fail("Transaction " + transactionId + " not found.");
    Transaction original = *found;
    if (original.getStatus() != TransactionStatus::COMPLETED)
        return fail("Only completed transactions can be reversed (status is " + original.getStatusString() + ").");
    if (trim(reason).empty())
        reason = "No reason given";

    if (original.isTransfer())
    {
        string outId = original.getTransactionType() == TransactionType::TRANSFER_OUT ? original.getTransactionId()
                                                                                      : original.getLinkedTransactionId();
        string inId = original.getTransactionType() == TransactionType::TRANSFER_IN ? original.getTransactionId()
                                                                                    : original.getLinkedTransactionId();
        const Transaction *outLeg = transactionManager->findTransaction(outId);
        const Transaction *inLeg = transactionManager->findTransaction(inId);
        if (!outLeg || !inLeg || outLeg->getStatus() != TransactionStatus::COMPLETED ||
            inLeg->getStatus() != TransactionStatus::COMPLETED)
            return fail("The other leg of this transfer is missing or already reversed.");
        Account *source = requireAccount(outLeg->getAccountNumber(), false);
        Account *destination = source ? requireAccount(inLeg->getAccountNumber(), false) : nullptr;
        if (!source || !destination)
            return fail("Cannot reverse: " + lastError);
        double amount = outLeg->getAmount();
        if (roundMoney(destination->getBalance() - amount) < lowestBalanceForAdminDebit(destination))
            return fail("Account " + destination->getAccountNumber() + " does not have enough funds to reverse this transfer.");

        double destBefore = destination->getBalance();
        double sourceBefore = source->getBalance();
        destination->updateBalance(destBefore - amount);
        source->updateBalance(sourceBefore + amount);
        string reversalId = transactionManager->processTransfer(destination->getAccountNumber(), source->getAccountNumber(),
                                                                amount, "Reversal of " + outId, destBefore,
                                                                destination->getBalance(), sourceBefore, source->getBalance(),
                                                                ownerOf(destination->getAccountNumber()),
                                                                ownerOf(source->getAccountNumber()));
        transactionManager->reverseTransaction(outId, reason + " (" + reversalId + ")");
        transactionManager->reverseTransaction(inId, reason + " (" + reversalId + ")");
        fileManager.logTransaction("Transfer " + outId + " reversed by " + reversalId + ": " + reason);
        persist();
        return true;
    }

    TransactionType type = original.getTransactionType();
    if (type != TransactionType::DEPOSIT && type != TransactionType::WITHDRAWAL &&
        type != TransactionType::FEE_CHARGE && type != TransactionType::INTEREST_CREDIT)
        return fail(original.getTransactionTypeString() + " transactions cannot be reversed.");

    Account *account = requireAccount(original.getAccountNumber(), false);
    if (!account)
        return fail("Cannot reverse: " + lastError);
    double amount = original.getAmount();
    double before = account->getBalance();
    TransactionType compensating;
    if (original.isCredit())
    {
        if (roundMoney(before - amount) < lowestBalanceForAdminDebit(account))
            return fail("Account " + account->getAccountNumber() + " does not have enough funds to reverse this transaction.");
        account->updateBalance(before - amount);
        compensating = TransactionType::WITHDRAWAL;
    }
    else
    {
        account->updateBalance(before + amount);
        compensating = TransactionType::DEPOSIT;
    }
    string reversalId = transactionManager->processTransaction(compensating, account->getAccountNumber(), amount,
                                                               "Reversal of " + transactionId, before,
                                                               account->getBalance(), ownerOf(account->getAccountNumber()));
    transactionManager->reverseTransaction(transactionId, reason + " (" + reversalId + ")");
    transactionManager->linkTransactions(transactionId, reversalId);
    fileManager.logTransaction("Transaction " + transactionId + " reversed by " + reversalId + ": " + reason);
    persist();
    return true;
}

// =============================================================================
// ACCOUNT SERVICES
// =============================================================================

bool Bank::freezeAccount(string accountNumber, string reason)
{
    Account *account = requireAccount(accountNumber, false);
    if (!account)
        return false;
    if (!account->isAccountActive())
        return fail("Account " + account->getAccountNumber() + " is already frozen.");
    account->setActive(false);
    fileManager.logSystem("Account " + account->getAccountNumber() + " frozen. Reason: " +
                          (trim(reason).empty() ? "not specified" : reason));
    persist();
    return true;
}

bool Bank::unfreezeAccount(string accountNumber)
{
    Account *account = requireAccount(accountNumber, false);
    if (!account)
        return false;
    if (account->isAccountActive())
        return fail("Account " + account->getAccountNumber() + " is not frozen.");
    Customer *owner = findCustomer(ownerOf(account->getAccountNumber()));
    if (owner && !owner->isCustomerActive())
        return fail("Owner " + owner->getCustomerId() + " is inactive. Reactivate the customer first.");
    account->setActive(true);
    fileManager.logSystem("Account " + account->getAccountNumber() + " unfrozen");
    persist();
    return true;
}

// Convert an account by opening one of the new type and moving the balance across
string Bank::changeAccountType(string accountNumber, AccountType newType)
{
    Account *account = requireAccount(accountNumber, true);
    if (!account)
        return "";
    if (account->getAccountType() == newType)
        return failId("Account is already a " + accountTypeToString(newType) + " account.");
    if (!activeLoansForAccount(account->getAccountNumber()).empty())
        return failId("Account is linked to an active loan and cannot be converted.");
    double balance = account->getBalance();
    double required = newType == AccountType::SAVINGS ? minSavingsBalance : minCheckingBalance;
    if (balance < required)
        return failId("A " + accountTypeToString(newType) + " account needs a balance of at least " +
                      formatCurrency(required) + ".");

    string oldNumber = account->getAccountNumber();
    string customerId = ownerOf(oldNumber);
    Customer *owner = findCustomer(customerId);

    // Settle this month's checking fees and carry the monthly activity count over,
    // so converting cannot be used to dodge fees or the savings withdrawal limit
    int monthlyActivity = 0;
    if (CheckingAccount *checking = dynamic_cast<CheckingAccount *>(account))
    {
        double feeBefore = checking->getBalance();
        double fee = checking->chargeFee();
        if (fee > 0)
            transactionManager->processFeeCharge(oldNumber, fee, "Transaction fees settled on conversion",
                                                 feeBefore, checking->getBalance(), customerId);
        monthlyActivity = checking->getTransactionCount();
    }
    else if (SavingsAccount *savings = dynamic_cast<SavingsAccount *>(account))
    {
        monthlyActivity = savings->getWithdrawalCount();
    }
    balance = account->getBalance();
    if (balance < required)
        return failId("After settling this month's fees the balance is below the " +
                      accountTypeToString(newType) + " minimum of " + formatCurrency(required) + ".");

    string newNumber;
    if (newType == AccountType::SAVINGS)
    {
        newNumber = generateSavingsAccountNumber();
        registerAccount(make_unique<SavingsAccount>(newNumber, customerId, 0.0, savingsInterestRate,
                                                    minSavingsBalance, maxSavingsWithdrawalsPerMonth),
                        owner);
    }
    else
    {
        newNumber = generateCheckingAccountNumber();
        registerAccount(make_unique<CheckingAccount>(newNumber, customerId, 0.0, defaultOverdraftLimit,
                                                     checkingTransactionFee, freeCheckingTransactions),
                        owner);
    }
    Account *newAccount = findAccount(newNumber);
    if (SavingsAccount *savings = dynamic_cast<SavingsAccount *>(newAccount))
        savings->setWithdrawalCount(monthlyActivity);
    else if (CheckingAccount *checking = dynamic_cast<CheckingAccount *>(newAccount))
        checking->setTransactionCount(monthlyActivity);
    transactionManager->processTransaction(TransactionType::ACCOUNT_OPENING, newNumber, 0.0,
                                           "Opened by conversion of " + oldNumber, 0.0, 0.0, customerId);
    account->updateBalance(0.0);
    newAccount->updateBalance(balance);
    transactionManager->processTransfer(oldNumber, newNumber, balance, "Account type conversion",
                                        balance, 0.0, 0.0, balance, customerId, customerId);
    transactionManager->processTransaction(TransactionType::ACCOUNT_CLOSING, oldNumber, 0.0,
                                           "Converted to " + newNumber, 0.0, 0.0, customerId);
    removeAccountRecord(oldNumber);
    totalAccountsCreated++;
    fileManager.logSystem("Account " + oldNumber + " converted to " + newNumber);
    persist();
    return newNumber;
}

bool Bank::setAccountOverdraftLimit(string accountNumber, double limit)
{
    Account *account = requireAccount(accountNumber, false);
    if (!account)
        return false;
    CheckingAccount *checking = dynamic_cast<CheckingAccount *>(account);
    if (!checking)
        return fail("Overdraft limits only apply to checking accounts.");
    if (limit < 0 || limit > maxTransferAmount)
        return fail("Overdraft limit must be between $0.00 and " + formatCurrency(maxTransferAmount) + ".");
    if (checking->getBalance() < -limit)
        return fail("Account is currently overdrawn by " + formatCurrency(-checking->getBalance()) +
                    "; the limit cannot be set below that.");
    checking->setOverdraftLimit(roundMoney(limit));
    fileManager.logSystem("Overdraft limit for " + checking->getAccountNumber() + " set to " + formatCurrency(limit));
    persist();
    return true;
}

double Bank::getAccountBalance(string accountNumber) const
{
    Account *account = findAccount(accountNumber);
    return account ? account->getBalance() : -1.0;
}

// =============================================================================
// LOANS
// =============================================================================

string Bank::applyForLoan(string customerId, string accountNumber, double amount, int termMonths)
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
        return "";
    if (!customer->isCustomerActive())
        return failId("Customer " + customer->getCustomerId() + " is inactive.");
    Account *account = requireAccount(accountNumber, true);
    if (!account)
        return "";
    if (account->getCustomerId() != customer->getCustomerId())
        return failId("Account " + account->getAccountNumber() + " does not belong to " + customer->getCustomerId() + ".");
    if (!isValidAmount(amount))
        return failId("Loan amount must be greater than zero.");
    amount = roundMoney(amount);
    if (amount < 100.0)
        return failId("The minimum loan amount is $100.00.");
    if (termMonths < 1 || termMonths > 360)
        return failId("Loan term must be between 1 and 360 months.");

    int activeLoans = 0;
    for (Loan *loan : getCustomerLoans(customer->getCustomerId()))
        if (loan->isActive())
            activeLoans++;
    if (activeLoans >= maxActiveLoansPerCustomer)
        return failId("Customers may hold at most " + to_string(maxActiveLoansPerCustomer) + " active loans.");
    double outstanding = getCustomerOutstandingLoans(customer->getCustomerId());
    if (outstanding + amount > maxLoanAmount)
        return failId("Total borrowing is capped at " + formatCurrency(maxLoanAmount) + ". You can borrow up to " +
                      formatCurrency(max(0.0, maxLoanAmount - outstanding)) + " more.");

    string loanId = generateLoanId();
    loans[loanId] = make_unique<Loan>(loanId, customer->getCustomerId(), account->getAccountNumber(), amount,
                                      loanInterestRate, termMonths);
    double before = account->getBalance();
    account->updateBalance(before + amount);
    transactionManager->processTransaction(TransactionType::LOAN_DISBURSEMENT, account->getAccountNumber(), amount,
                                           "Loan disbursement " + loanId, before, account->getBalance(),
                                           customer->getCustomerId());
    fileManager.logTransaction("Loan " + loanId + " of " + formatCurrency(amount) + " disbursed to " +
                               account->getAccountNumber());
    persist();
    return loanId;
}

bool Bank::makeLoanPayment(string loanId, double amount)
{
    Loan *loan = findLoan(loanId);
    if (!loan)
        return fail("Loan " + loanId + " not found.");
    if (!loan->isActive())
        return fail("Loan " + loan->getLoanId() + " is " + loan->getStatusString() + ".");
    if (!isValidAmount(amount))
        return fail("Payment amount must be greater than zero.");
    Account *account = requireAccount(loan->getLinkedAccountNumber(), true);
    if (!account)
        return false;
    amount = roundMoney(min(amount, loan->getOutstandingBalance()));
    double before = account->getBalance();
    if (roundMoney(before - amount) < account->getLowestAllowedBalance())
        return fail("Insufficient funds in " + account->getAccountNumber() + " for a payment of " +
                    formatCurrency(amount) + ".");

    account->updateBalance(before - amount);
    loan->makePayment(amount);
    transactionManager->processTransaction(TransactionType::LOAN_PAYMENT, account->getAccountNumber(), amount,
                                           "Loan payment " + loan->getLoanId(), before, account->getBalance(),
                                           loan->getCustomerId());
    fileManager.logTransaction("Loan " + loan->getLoanId() + " payment of " + formatCurrency(amount) +
                               (loan->isActive() ? "" : " (paid off)"));
    persist();
    return true;
}

Loan *Bank::findLoan(string loanId) const
{
    auto it = loans.find(toUpper(trim(loanId)));
    return it != loans.end() ? it->second.get() : nullptr;
}

vector<Loan *> Bank::getCustomerLoans(string customerId) const
{
    vector<Loan *> result;
    string wanted = toUpper(trim(customerId));
    for (const auto &pair : loans)
    {
        if (pair.second->getCustomerId() == wanted)
            result.push_back(pair.second.get());
    }
    return result;
}

vector<Loan *> Bank::getAllLoans() const
{
    vector<Loan *> result;
    for (const auto &pair : loans)
        result.push_back(pair.second.get());
    return result;
}

double Bank::getCustomerOutstandingLoans(string customerId) const
{
    double total = 0.0;
    for (Loan *loan : getCustomerLoans(customerId))
        if (loan->isActive())
            total += loan->getOutstandingBalance();
    return roundMoney(total);
}

double Bank::getTotalOutstandingLoans() const
{
    double total = 0.0;
    for (const auto &pair : loans)
        if (pair.second->isActive())
            total += pair.second->getOutstandingBalance();
    return roundMoney(total);
}

static void displayLoanTable(const vector<Loan *> &loanList)
{
    cout << left << setw(10) << "Loan ID" << setw(12) << "Customer" << setw(13) << "Account"
         << right << setw(14) << "Principal" << setw(14) << "Outstanding" << setw(12) << "EMI"
         << setw(7) << "Rate" << "  " << left << "Status" << endl;
    cout << string(92, '-') << endl;
    for (const Loan *loan : loanList)
    {
        stringstream rateText;
        rateText << fixed << setprecision(1) << loan->getAnnualInterestRate() * 100 << "%";
        cout << left << setw(10) << loan->getLoanId() << setw(12) << loan->getCustomerId()
             << setw(13) << loan->getLinkedAccountNumber()
             << right << setw(14) << formatCurrency(loan->getPrincipal())
             << setw(14) << formatCurrency(loan->getOutstandingBalance())
             << setw(12) << formatCurrency(loan->getMonthlyPayment())
             << setw(7) << rateText.str() << "  " << left << loan->getStatusString() << endl;
    }
    if (loanList.empty())
        cout << "No loans found." << endl;
}

void Bank::displayAllLoans() const
{
    cout << "All Loans" << endl;
    cout << string(92, '=') << endl;
    displayLoanTable(getAllLoans());
    cout << string(92, '-') << endl;
    cout << "Total outstanding: " << formatCurrency(getTotalOutstandingLoans()) << endl;
}

void Bank::displayCustomerLoans(string customerId) const
{
    vector<Loan *> customerLoans = getCustomerLoans(customerId);
    displayLoanTable(customerLoans);
    if (!customerLoans.empty())
        cout << "Total outstanding: " << formatCurrency(getCustomerOutstandingLoans(customerId)) << endl;
}

// =============================================================================
// TRANSACTION HISTORY & ANALYTICS
// =============================================================================

vector<Transaction> Bank::getAccountTransactionHistory(string accountNumber, int limit) const
{
    return transactionManager->getAccountHistory(toUpper(trim(accountNumber)), limit);
}

vector<Transaction> Bank::getCustomerTransactionHistory(string customerId, int limit) const
{
    return transactionManager->getCustomerHistory(toUpper(trim(customerId)), limit);
}

double Bank::getAccountNetFlow(string accountNumber, string startDate, string endDate) const
{
    return transactionManager->getNetFlow(toUpper(trim(accountNumber)), startDate, endDate);
}

bool Bank::exportAccountStatement(string accountNumber, string filename) const
{
    if (!findAccount(accountNumber) && transactionManager->getAccountHistory(accountNumber, 1).empty())
        return fail("Account " + accountNumber + " not found.");
    if (!transactionManager->exportTransactionsToCSV(filename, toUpper(trim(accountNumber))))
        return fail("Could not write " + filename + ".");
    return true;
}

const TransactionManager &Bank::getTransactionManager() const
{
    return *transactionManager;
}

void Bank::setSessionContext(string sessionId)
{
    transactionManager->setSessionContext(sessionId);
}

// =============================================================================
// SYSTEM ADMINISTRATION
// =============================================================================

void Bank::displaySystemStatistics() const
{
    displayReportHeader("System Statistics");
    int customerCount = getTotalCustomers();
    int accountCount = getTotalAccounts();
    double systemBalance = getTotalSystemBalance();
    cout << left << setw(34) << "Customers (active/inactive)" << ": " << customerCount << " ("
         << getActiveCustomerCount() << "/" << getInactiveCustomerCount() << ")" << endl;
    cout << left << setw(34) << "Accounts (savings/checking)" << ": " << accountCount << " ("
         << getSavingsAccountCount() << "/" << getCheckingAccountCount() << ")" << endl;
    cout << left << setw(34) << "Frozen accounts" << ": " << getTotalInactiveAccounts() << endl;
    cout << left << setw(34) << "Total deposits held" << ": " << formatCurrency(systemBalance) << endl;
    cout << left << setw(34) << "Average account balance" << ": "
         << formatCurrency(accountCount > 0 ? systemBalance / accountCount : 0.0) << endl;
    cout << left << setw(34) << "Average accounts per customer" << ": " << fixed << setprecision(2)
         << (customerCount > 0 ? static_cast<double>(accountCount) / customerCount : 0.0) << endl;
    cout << left << setw(34) << "Active loans outstanding" << ": " << formatCurrency(getTotalOutstandingLoans()) << endl;
    cout << left << setw(34) << "Lifetime deposits processed" << ": " << formatCurrency(totalDeposits) << endl;
    cout << left << setw(34) << "Lifetime withdrawals processed" << ": " << formatCurrency(totalWithdrawals) << endl;
    cout << left << setw(34) << "Customers / accounts ever created" << ": " << totalCustomersCreated << " / "
         << totalAccountsCreated << endl;
    cout << left << setw(34) << "Transactions recorded" << ": " << getTotalTransactions() << endl;
    cout << left << setw(34) << "Transaction volume" << ": " << formatCurrency(transactionManager->getTotalSystemVolume()) << endl;
    cout << left << setw(34) << "Last month-end run" << ": "
         << (lastMonthEndProcessed.empty() ? "never" : lastMonthEndProcessed) << endl;
}

void Bank::displayAllCustomers() const
{
    cout << "Customer List" << endl;
    cout << string(86, '=') << endl;
    cout << left << setw(13) << "Customer ID" << setw(26) << "Name" << setw(28) << "Email"
         << right << setw(4) << "Acc" << setw(15) << "Balance" << endl;
    cout << string(86, '-') << endl;
    for (const auto &pair : customers)
    {
        const Customer *customer = pair.second.get();
        string name = customer->getName();
        if (!customer->isCustomerActive())
            name += " [inactive]";
        else if (customer->isAccountLocked())
            name += " [locked]";
        cout << left << setw(13) << customer->getCustomerId()
             << setw(26) << name.substr(0, 25)
             << setw(28) << customer->getEmail().substr(0, 27)
             << right << setw(4) << customer->getAccountCount()
             << setw(15) << formatCurrency(customer->getTotalBalance()) << endl;
    }
    if (customers.empty())
        cout << "No customers registered." << endl;
    cout << string(86, '-') << endl;
    cout << "Total customers: " << customers.size() << " (" << getActiveCustomerCount() << " active)"
         << "   Total balance: " << formatCurrency(getTotalSystemBalance()) << endl;
}

void Bank::displayAllAccounts() const
{
    cout << "Account List" << endl;
    cout << string(70, '=') << endl;
    cout << left << setw(14) << "Account" << setw(11) << "Type" << setw(13) << "Customer"
         << right << setw(16) << "Balance" << "   " << left << setw(8) << "Status" << endl;
    cout << string(70, '-') << endl;
    for (const auto &pair : accounts)
    {
        const Account *account = pair.second.get();
        cout << left << setw(14) << account->getAccountNumber()
             << setw(11) << account->getAccountTypeString()
             << setw(13) << ownerOf(pair.first)
             << right << setw(16) << formatCurrency(account->getBalance())
             << "   " << left << setw(8) << (account->isAccountActive() ? "Active" : "Frozen") << endl;
    }
    if (accounts.empty())
        cout << "No accounts open." << endl;
    cout << string(70, '-') << endl;
    cout << "Total: " << accounts.size() << " (" << getSavingsAccountCount() << " savings, "
         << getCheckingAccountCount() << " checking, " << getTotalInactiveAccounts() << " frozen)"
         << "   Balance: " << formatCurrency(getTotalSystemBalance()) << endl;
}

void Bank::displayCustomerDetails(string customerId) const
{
    Customer *customer = findCustomer(customerId);
    if (!customer)
    {
        cout << lastError << endl;
        return;
    }
    customer->displayCustomerInfo();
    cout << endl;
    customer->displayAccountSummary();
    cout << "\nLoans" << endl;
    displayCustomerLoans(customer->getCustomerId());
    cout << "\nRecent Transactions" << endl;
    TransactionManager::displayTransactionTable(getCustomerTransactionHistory(customer->getCustomerId(), 10));
}

void Bank::displayLargeTransactions(double minAmount) const
{
    vector<Transaction> largeTransactions = transactionManager->getLargeTransactions(minAmount);
    cout << "Transactions of " << formatCurrency(minAmount) << " or more" << endl;
    cout << string(110, '=') << endl;
    TransactionManager::displayTransactionTable(largeTransactions);
    double totalAmount = 0.0;
    for (const auto &transaction : largeTransactions)
        totalAmount += transaction.getAmount();
    cout << string(110, '-') << endl;
    cout << "Count: " << largeTransactions.size() << "   Total: " << formatCurrency(totalAmount);
    if (!largeTransactions.empty())
        cout << "   Average: " << formatCurrency(totalAmount / largeTransactions.size());
    cout << endl;
}

void Bank::displayInactiveAccounts() const
{
    cout << "Frozen / Inactive Accounts" << endl;
    cout << string(60, '=') << endl;
    int inactiveCount = 0;
    for (const auto &pair : accounts)
    {
        const Account *account = pair.second.get();
        if (account->isAccountActive())
            continue;
        cout << left << setw(14) << account->getAccountNumber()
             << setw(11) << account->getAccountTypeString()
             << setw(13) << ownerOf(pair.first)
             << right << setw(16) << formatCurrency(account->getBalance()) << endl;
        inactiveCount++;
    }
    if (inactiveCount == 0)
        cout << "No frozen accounts." << endl;
    cout << string(60, '-') << endl;
    cout << "Total frozen accounts: " << inactiveCount << endl;
}

void Bank::displayCustomersWithMultipleAccounts() const
{
    cout << "Customers with Multiple Accounts" << endl;
    cout << string(60, '=') << endl;
    int count = 0;
    for (const auto &pair : customers)
    {
        const Customer *customer = pair.second.get();
        if (customer->getAccountCount() < 2)
            continue;
        count++;
        cout << customer->getCustomerId() << "  " << customer->getName() << "  ("
             << customer->getAccountCount() << " accounts, " << formatCurrency(customer->getTotalBalance()) << ")" << endl;
        for (const Account *account : customer->getAllAccounts())
        {
            cout << "    - " << left << setw(13) << account->getAccountNumber()
                 << setw(10) << account->getAccountTypeString()
                 << right << setw(16) << formatCurrency(account->getBalance()) << endl;
        }
    }
    if (count == 0)
        cout << "No customers hold more than one account." << endl;
    cout << string(60, '-') << endl;
    cout << "Total: " << count << endl;
}

// =============================================================================
// SYSTEM MAINTENANCE
// =============================================================================

void Bank::performSystemMaintenance()
{
    cout << "Running system maintenance..." << endl;
    vector<string> issues = findIntegrityIssues();
    if (issues.empty())
    {
        cout << "[OK] Integrity check passed." << endl;
    }
    else
    {
        cout << "[!!] Integrity check found " << issues.size() << " issue(s):" << endl;
        for (const string &issue : issues)
            cout << "     - " << issue << endl;
        int fixes = fixDataInconsistencies();
        cout << "[OK] Applied " << fixes << " automatic repair(s)." << endl;
        vector<string> remaining = findIntegrityIssues();
        if (!remaining.empty())
            cout << "[!!] " << remaining.size() << " issue(s) need manual review (see logs/errors.log)." << endl;
        for (const string &issue : remaining)
            fileManager.logError("Integrity: " + issue);
    }

    int removedLogs = fileManager.cleanupOldLogs(logRetentionDays);
    cout << "[OK] Removed " << removedLogs << " rotated log file(s) older than " << logRetentionDays << " days." << endl;

    string backupName;
    if (backupSystemData(backupName))
        cout << "[OK] Backup created: " << fileManager.getBackupDirectory() << "/" << backupName << endl;
    else
        cout << "[!!] Backup failed: " << lastError << endl;

    cout << string(50, '-') << endl;
    cout << "Customers: " << getTotalCustomers() << "   Accounts: " << getTotalAccounts()
         << "   Loans: " << loans.size() << "   Transactions: " << getTotalTransactions() << endl;
    fileManager.logSystem("System maintenance completed");
}

void Bank::generateDailyReport(string date) const
{
    if (!isValidDate(date))
    {
        cout << "Invalid date. Use YYYY-MM-DD." << endl;
        return;
    }
    displayReportHeader("Daily Report for " + date);
    vector<Transaction> transactions = transactionManager->getTransactionsByDateRange(date, date);

    map<TransactionType, pair<int, double>> byType;
    double credits = 0.0, debits = 0.0;
    for (const Transaction &txn : transactions)
    {
        if (!txn.affectsBalance())
            continue;
        byType[txn.getTransactionType()].first++;
        byType[txn.getTransactionType()].second += txn.getAmount();
        if (txn.isCredit())
            credits += txn.getAmount();
        else
            debits += txn.getAmount();
    }

    cout << "\nActivity by type" << endl;
    cout << left << setw(20) << "Type" << right << setw(8) << "Count" << setw(18) << "Amount" << endl;
    cout << string(46, '-') << endl;
    for (const auto &[type, stats] : byType)
        cout << left << setw(20) << Transaction::typeToString(type) << right << setw(8) << stats.first
             << setw(18) << formatCurrency(stats.second) << endl;
    if (byType.empty())
        cout << "No transactions on this date." << endl;
    cout << string(46, '-') << endl;
    cout << "Total credits : " << formatCurrency(credits) << endl;
    cout << "Total debits  : " << formatCurrency(debits) << endl;
    cout << "Net flow      : " << formatCurrency(credits - debits) << endl;

    cout << "\nNew customers" << endl;
    int newCustomers = 0;
    for (const auto &pair : customers)
    {
        if (pair.second->getRegistrationDate() == date)
        {
            cout << "  " << pair.first << "  " << pair.second->getName() << endl;
            newCustomers++;
        }
    }
    if (newCustomers == 0)
        cout << "  None" << endl;

    cout << "\nNew accounts" << endl;
    int newAccounts = 0;
    for (const auto &pair : accounts)
    {
        if (pair.second->getCreationDate() == date)
        {
            cout << "  " << left << setw(13) << pair.first << setw(10) << pair.second->getAccountTypeString()
                 << ownerOf(pair.first) << endl;
            newAccounts++;
        }
    }
    if (newAccounts == 0)
        cout << "  None" << endl;

    cout << "\nLargest transactions" << endl;
    vector<Transaction> largest = transactions;
    sort(largest.begin(), largest.end(), [](const Transaction &a, const Transaction &b)
         { return a.getAmount() > b.getAmount(); });
    if (largest.size() > 5)
        largest.resize(5);
    TransactionManager::displayTransactionTable(largest);
}

void Bank::generateMonthlyReport(string month) const
{
    if (!isValidMonth(month))
    {
        cout << "Invalid month. Use YYYY-MM." << endl;
        return;
    }
    displayReportHeader("Monthly Report for " + month);
    vector<Transaction> transactions = transactionManager->getTransactionsByDateRange(month + "-01", month + "-31");

    map<TransactionType, pair<int, double>> byType;
    map<string, double> volumeByCustomer;
    map<string, int> activeDays;
    double credits = 0.0, debits = 0.0;
    for (const Transaction &txn : transactions)
    {
        if (!txn.affectsBalance())
            continue;
        byType[txn.getTransactionType()].first++;
        byType[txn.getTransactionType()].second += txn.getAmount();
        if (txn.isCredit())
            credits += txn.getAmount();
        else
            debits += txn.getAmount();
        if (!txn.getCustomerId().empty())
            volumeByCustomer[txn.getCustomerId()] += txn.getAmount();
        activeDays[txn.getDate()]++;
    }

    cout << "\nSummary" << endl;
    cout << "  Transactions      : " << transactions.size() << " over " << activeDays.size() << " active day(s)" << endl;
    cout << "  Total credits     : " << formatCurrency(credits) << endl;
    cout << "  Total debits      : " << formatCurrency(debits) << endl;
    cout << "  Net flow          : " << formatCurrency(credits - debits) << endl;
    auto typeTotal = [&byType](TransactionType type)
    { auto it = byType.find(type); return it == byType.end() ? 0.0 : it->second.second; };
    cout << "  Interest paid     : " << formatCurrency(typeTotal(TransactionType::INTEREST_CREDIT)) << endl;
    cout << "  Fees collected    : " << formatCurrency(typeTotal(TransactionType::FEE_CHARGE)) << endl;
    cout << "  Loans disbursed   : " << formatCurrency(typeTotal(TransactionType::LOAN_DISBURSEMENT)) << endl;
    cout << "  Loan repayments   : " << formatCurrency(typeTotal(TransactionType::LOAN_PAYMENT)) << endl;

    cout << "\nActivity by type" << endl;
    cout << left << setw(20) << "Type" << right << setw(8) << "Count" << setw(18) << "Amount" << endl;
    cout << string(46, '-') << endl;
    for (const auto &[type, stats] : byType)
        cout << left << setw(20) << Transaction::typeToString(type) << right << setw(8) << stats.first
             << setw(18) << formatCurrency(stats.second) << endl;
    if (byType.empty())
        cout << "No transactions this month." << endl;

    int newCustomers = 0, newAccounts = 0, newSavings = 0;
    for (const auto &pair : customers)
        if (pair.second->getRegistrationDate().compare(0, 7, month) == 0)
            newCustomers++;
    for (const auto &pair : accounts)
    {
        if (pair.second->getCreationDate().compare(0, 7, month) == 0)
        {
            newAccounts++;
            if (pair.second->getAccountType() == AccountType::SAVINGS)
                newSavings++;
        }
    }
    cout << "\nGrowth" << endl;
    cout << "  New customers     : " << newCustomers << " (total now " << customers.size() << ")" << endl;
    cout << "  New accounts      : " << newAccounts << " (" << newSavings << " savings, "
         << newAccounts - newSavings << " checking; total now " << accounts.size() << ")" << endl;
    cout << "  Deposits held now : " << formatCurrency(getTotalSystemBalance()) << endl;

    cout << "\nTop customers by transaction volume" << endl;
    vector<pair<string, double>> ranking(volumeByCustomer.begin(), volumeByCustomer.end());
    sort(ranking.begin(), ranking.end(), [](const auto &a, const auto &b)
         { return a.second > b.second; });
    int rank = 0;
    for (const auto &[customerId, volume] : ranking)
    {
        if (++rank > 5)
            break;
        auto it = customers.find(customerId);
        string name = it != customers.end() ? it->second->getName() : "(closed)";
        cout << "  " << rank << ". " << left << setw(12) << customerId << setw(26) << name.substr(0, 25)
             << right << setw(16) << formatCurrency(volume) << endl;
    }
    if (ranking.empty())
        cout << "  None" << endl;
}

bool Bank::backupSystemData(string &backupName) const
{
    if (!saveAllData())
        return false;
    if (!fileManager.createBackup(backupName))
        return fail("Could not create a backup in " + fileManager.getBackupDirectory() + ".");
    fileManager.logSystem("Backup created: " + backupName);
    return true;
}

bool Bank::restoreSystemData(string backupName)
{
    vector<string> backups = fileManager.listBackups();
    if (backups.empty())
        return fail("No backups found in " + fileManager.getBackupDirectory() + ".");
    if (backupName.empty())
        backupName = backups.front();
    if (find(backups.begin(), backups.end(), backupName) == backups.end())
        return fail("Backup " + backupName + " not found.");

    // Keep a safety copy of the current state before overwriting it
    string safetyBackup;
    if (saveAllData())
        fileManager.createBackup(safetyBackup);

    if (!fileManager.restoreBackup(backupName))
        return fail("Could not restore backup " + backupName + ".");
    if (!loadAllData())
        return false;
    fileManager.logSystem("Restored backup " + backupName +
                          (safetyBackup.empty() ? "" : " (previous state saved as " + safetyBackup + ")"));
    return true;
}

vector<string> Bank::listBackups() const
{
    return fileManager.listBackups();
}

// =============================================================================
// SYSTEM CONFIGURATION
// =============================================================================

bool Bank::loadConfiguration(string path)
{
    configFilePath = path;
    map<string, string> config;
    if (!FileManager::loadKeyValueFile(path, config))
        return fail("Configuration file " + path + " not found; using defaults.");

    auto getDouble = [&config](const string &key, double current, double minValue, double maxValue)
    {
        auto it = config.find(key);
        if (it == config.end())
            return current;
        double value = parseDouble(it->second, current);
        return (value >= minValue && value <= maxValue) ? value : current;
    };
    auto getInt = [&config](const string &key, int current, int minValue, int maxValue)
    {
        auto it = config.find(key);
        if (it == config.end())
            return current;
        int value = parseInt(it->second, current);
        return (value >= minValue && value <= maxValue) ? value : current;
    };

    minSavingsBalance = getDouble("MIN_SAVINGS_BALANCE", minSavingsBalance, 0, 1e9);
    minCheckingBalance = getDouble("MIN_CHECKING_BALANCE", minCheckingBalance, 0, 1e9);
    savingsInterestRate = getDouble("SAVINGS_INTEREST_RATE", savingsInterestRate, 0, 0.20);
    loanInterestRate = getDouble("LOAN_INTEREST_RATE", loanInterestRate, 0, 0.50);
    maxDailyWithdrawal = getDouble("MAX_DAILY_WITHDRAWAL", maxDailyWithdrawal, 1, 1e9);
    maxTransferAmount = getDouble("MAX_TRANSFER_AMOUNT", maxTransferAmount, 1, 1e9);
    maxSavingsWithdrawalsPerMonth = getInt("MAX_WITHDRAWAL_PER_MONTH", maxSavingsWithdrawalsPerMonth, 0, 1000);
    defaultOverdraftLimit = getDouble("OVERDRAFT_LIMIT", defaultOverdraftLimit, 0, 1e9);
    checkingTransactionFee = getDouble("TRANSACTION_FEE", checkingTransactionFee, 0, 1000);
    freeCheckingTransactions = getInt("FREE_CHECKING_TRANSACTIONS", freeCheckingTransactions, 0, 1000);
    maxLoanAmount = getDouble("MAX_LOAN_AMOUNT", maxLoanAmount, 100, 1e9);
    maxActiveLoansPerCustomer = getInt("MAX_ACTIVE_LOANS", maxActiveLoansPerCustomer, 1, 100);
    sessionTimeoutSeconds = getInt("SESSION_TIMEOUT", sessionTimeoutSeconds, 0, 86400 * 7);
    maxLoginAttempts = getInt("MAX_LOGIN_ATTEMPTS", maxLoginAttempts, 1, 100);
    backupIntervalHours = getInt("BACKUP_INTERVAL", backupIntervalHours, 0, 24 * 365);
    return true;
}

bool Bank::saveConfiguration() const
{
    if (configFilePath.empty())
        return true; // Running without a config file (e.g. tests)
    const vector<pair<string, string>> values = {
        {"MIN_SAVINGS_BALANCE", moneyField(minSavingsBalance)},
        {"MIN_CHECKING_BALANCE", moneyField(minCheckingBalance)},
        {"SAVINGS_INTEREST_RATE", rateField(savingsInterestRate)},
        {"LOAN_INTEREST_RATE", rateField(loanInterestRate)},
        {"MAX_DAILY_WITHDRAWAL", moneyField(maxDailyWithdrawal)},
        {"MAX_TRANSFER_AMOUNT", moneyField(maxTransferAmount)},
        {"OVERDRAFT_LIMIT", moneyField(defaultOverdraftLimit)},
    };
    for (const auto &[key, value] : values)
    {
        if (!FileManager::updateKeyValueFile(configFilePath, key, value))
            return fail("Could not update " + configFilePath + ".");
    }
    return true;
}

bool Bank::setMinimumBalances(double savingsMin, double checkingMin)
{
    if (savingsMin < 0 || checkingMin < 0 || !std::isfinite(savingsMin) || !std::isfinite(checkingMin))
        return fail("Minimum balances cannot be negative.");
    minSavingsBalance = roundMoney(savingsMin);
    minCheckingBalance = roundMoney(checkingMin);
    for (const auto &pair : accounts)
        if (SavingsAccount *savings = dynamic_cast<SavingsAccount *>(pair.second.get()))
            savings->setMinimumBalance(minSavingsBalance);
    fileManager.logSystem("Minimum balances set: savings " + formatCurrency(minSavingsBalance) +
                          ", checking " + formatCurrency(minCheckingBalance));
    bool saved = saveConfiguration();
    persist();
    return saved;
}

bool Bank::setDefaultOverdraftLimit(double limit)
{
    if (limit < 0 || !std::isfinite(limit))
        return fail("Overdraft limit cannot be negative.");
    defaultOverdraftLimit = roundMoney(limit);
    fileManager.logSystem("Default overdraft limit set to " + formatCurrency(defaultOverdraftLimit));
    return saveConfiguration();
}

bool Bank::setSavingsInterestRate(double rate)
{
    if (rate < 0.0 || rate > 0.20 || !std::isfinite(rate))
        return fail("Savings interest rate must be between 0% and 20%.");
    savingsInterestRate = rate;
    for (const auto &pair : accounts)
        if (SavingsAccount *savings = dynamic_cast<SavingsAccount *>(pair.second.get()))
            savings->setInterestRate(rate);
    fileManager.logSystem("Savings interest rate set to " + percent(rate));
    bool saved = saveConfiguration();
    persist();
    return saved;
}

bool Bank::setLoanInterestRate(double rate)
{
    if (rate < 0.0 || rate > 0.50 || !std::isfinite(rate))
        return fail("Loan interest rate must be between 0% and 50%.");
    loanInterestRate = rate;
    fileManager.logSystem("Loan interest rate for new loans set to " + percent(rate));
    return saveConfiguration();
}

bool Bank::setTransactionLimits(double dailyWithdrawal, double transferAmount)
{
    if (dailyWithdrawal <= 0 || transferAmount <= 0 || !std::isfinite(dailyWithdrawal) || !std::isfinite(transferAmount))
        return fail("Limits must be greater than zero.");
    maxDailyWithdrawal = roundMoney(dailyWithdrawal);
    maxTransferAmount = roundMoney(transferAmount);
    fileManager.logSystem("Limits set: daily withdrawal " + formatCurrency(maxDailyWithdrawal) +
                          ", transfer " + formatCurrency(maxTransferAmount));
    return saveConfiguration();
}

void Bank::displaySystemConfiguration() const
{
    displayReportHeader("System Configuration");
    cout << left << setw(34) << "Minimum savings balance" << ": " << formatCurrency(minSavingsBalance) << endl;
    cout << left << setw(34) << "Minimum checking balance" << ": " << formatCurrency(minCheckingBalance) << endl;
    cout << left << setw(34) << "Savings interest rate" << ": " << percent(savingsInterestRate) << " p.a." << endl;
    cout << left << setw(34) << "Savings withdrawals per month" << ": " << maxSavingsWithdrawalsPerMonth << endl;
    cout << left << setw(34) << "Default overdraft limit" << ": " << formatCurrency(defaultOverdraftLimit) << endl;
    cout << left << setw(34) << "Checking fee per extra transaction" << ": " << formatCurrency(checkingTransactionFee)
         << " (after " << freeCheckingTransactions << " free)" << endl;
    cout << left << setw(34) << "Daily withdrawal limit" << ": " << formatCurrency(maxDailyWithdrawal) << endl;
    cout << left << setw(34) << "Maximum single transfer" << ": " << formatCurrency(maxTransferAmount) << endl;
    cout << left << setw(34) << "Loan interest rate" << ": " << percent(loanInterestRate) << " p.a." << endl;
    cout << left << setw(34) << "Maximum borrowing per customer" << ": " << formatCurrency(maxLoanAmount)
         << " (" << maxActiveLoansPerCustomer << " loans max)" << endl;
    cout << left << setw(34) << "Max login attempts" << ": " << maxLoginAttempts << endl;
    cout << left << setw(34) << "Customer session timeout" << ": " << sessionTimeoutSeconds << " s" << endl;
    cout << left << setw(34) << "Backup interval" << ": " << backupIntervalHours << " h" << endl;
    cout << left << setw(34) << "Config file" << ": " << (configFilePath.empty() ? "(none)" : configFilePath) << endl;
    cout << left << setw(34) << "Data directory" << ": " << fileManager.getDataDirectory() << endl;
}

void Bank::setLogRetentionDays(int days)
{
    if (days > 0)
        logRetentionDays = days;
}

int Bank::getMaxLoginAttempts() const { return maxLoginAttempts; }
int Bank::getSessionTimeout() const { return sessionTimeoutSeconds; }
int Bank::getBackupInterval() const { return backupIntervalHours; }
double Bank::getMinSavingsBalance() const { return minSavingsBalance; }
double Bank::getMinCheckingBalance() const { return minCheckingBalance; }
double Bank::getDefaultOverdraftLimit() const { return defaultOverdraftLimit; }
double Bank::getLoanInterestRate() const { return loanInterestRate; }
double Bank::getMaxLoanAmount() const { return maxLoanAmount; }

// =============================================================================
// DATA PERSISTENCE
// =============================================================================

bool Bank::saveAllData() const
{
    vector<vector<string>> customerRecords;
    for (const auto &pair : customers)
    {
        const Customer *c = pair.second.get();
        customerRecords.push_back({c->customerId, c->name, c->address, c->phoneNumber, c->email, c->hashedPin,
                                   c->registrationDate, boolField(c->isActive), to_string(c->failedLoginAttempts),
                                   boolField(c->isLocked)});
    }

    vector<vector<string>> accountRecords;
    for (const auto &pair : accounts)
    {
        const Account *a = pair.second.get();
        vector<string> record = {a->getAccountNumber(), a->getAccountTypeString(), a->getCustomerId(),
                                 moneyField(a->getBalance()), a->getCreationDate(), boolField(a->isAccountActive())};
        if (const SavingsAccount *s = dynamic_cast<const SavingsAccount *>(a))
        {
            record.insert(record.end(), {rateField(s->getInterestRate()), moneyField(s->getMinimumBalance()),
                                         to_string(s->getWithdrawalCount()), to_string(s->getMaxWithdrawals())});
        }
        else if (const CheckingAccount *ch = dynamic_cast<const CheckingAccount *>(a))
        {
            record.insert(record.end(), {moneyField(ch->getOverdraftLimit()), moneyField(ch->getTransactionFee()),
                                         to_string(ch->getFreeTransactions()), to_string(ch->getTransactionCount())});
        }
        accountRecords.push_back(record);
    }

    vector<vector<string>> loanRecords;
    for (const auto &pair : loans)
    {
        const Loan *l = pair.second.get();
        loanRecords.push_back({l->getLoanId(), l->getCustomerId(), l->getLinkedAccountNumber(),
                               moneyField(l->getPrincipal()), rateField(l->getAnnualInterestRate()),
                               to_string(l->getTermMonths()), moneyField(l->getMonthlyPayment()),
                               moneyField(l->getOutstandingBalance()), moneyField(l->getTotalPaid()),
                               moneyField(l->getTotalInterestAccrued()), to_string(l->getPaymentsMade()),
                               l->getStartDate(), l->getStatusString()});
    }

    map<string, string> systemState = {
        {"BANK_NAME", bankName},
        {"BANK_CODE", bankCode},
        {"NEXT_CUSTOMER_NUMBER", to_string(nextCustomerNumber)},
        {"NEXT_SAVINGS_NUMBER", to_string(nextSavingsAccountNumber)},
        {"NEXT_CHECKING_NUMBER", to_string(nextCheckingAccountNumber)},
        {"NEXT_LOAN_NUMBER", to_string(nextLoanNumber)},
        {"TOTAL_DEPOSITS", moneyField(totalDeposits)},
        {"TOTAL_WITHDRAWALS", moneyField(totalWithdrawals)},
        {"TOTAL_CUSTOMERS_CREATED", to_string(totalCustomersCreated)},
        {"TOTAL_ACCOUNTS_CREATED", to_string(totalAccountsCreated)},
        {"LAST_MONTH_END", lastMonthEndProcessed},
        {"LAST_SAVED", getCurrentDateTime()},
    };

    bool ok = fileManager.writeRecords("customers.dat",
                                       "id|name|address|phone|email|pin_hash|registered|active|failed_logins|locked",
                                       customerRecords) &&
              fileManager.writeRecords("accounts.dat",
                                       "number|type|customer|balance|created|active|type-specific fields...",
                                       accountRecords) &&
              fileManager.writeRecords("loans.dat",
                                       "id|customer|account|principal|rate|term|emi|outstanding|paid|interest|payments|start|status",
                                       loanRecords) &&
              transactionManager->saveTransactionHistory() &&
              FileManager::writeKeyValueFile(fileManager.getDataFilePath("system_config.dat"), systemState,
                                             "Banking system state - generated, do not edit by hand");
    if (!ok)
    {
        fileManager.logError("Failed to save data to " + fileManager.getDataDirectory());
        return fail("Could not save data to " + fileManager.getDataDirectory() + ".");
    }
    return true;
}

bool Bank::loadAllData()
{
    // Start from a clean slate so a reload/restore never mixes old and new data
    lastError.clear();
    customers.clear();
    accounts.clear();
    loans.clear();
    accountCustomerMap.clear();
    transactionManager->clear();
    nextCustomerNumber = nextSavingsAccountNumber = nextCheckingAccountNumber = nextLoanNumber = 1;
    totalDeposits = totalWithdrawals = 0.0;
    totalCustomersCreated = totalAccountsCreated = 0;
    lastMonthEndProcessed = "";

    int skipped = 0;
    map<string, string> state;
    if (FileManager::loadKeyValueFile(fileManager.getDataFilePath("system_config.dat"), state))
    {
        nextCustomerNumber = parseInt(state["NEXT_CUSTOMER_NUMBER"], 1);
        nextSavingsAccountNumber = parseInt(state["NEXT_SAVINGS_NUMBER"], 1);
        nextCheckingAccountNumber = parseInt(state["NEXT_CHECKING_NUMBER"], 1);
        nextLoanNumber = parseInt(state["NEXT_LOAN_NUMBER"], 1);
        totalDeposits = parseDouble(state["TOTAL_DEPOSITS"]);
        totalWithdrawals = parseDouble(state["TOTAL_WITHDRAWALS"]);
        totalCustomersCreated = parseInt(state["TOTAL_CUSTOMERS_CREATED"]);
        totalAccountsCreated = parseInt(state["TOTAL_ACCOUNTS_CREATED"]);
        lastMonthEndProcessed = state["LAST_MONTH_END"];
    }

    vector<vector<string>> records;
    if (fileManager.readRecords("customers.dat", records))
    {
        for (const auto &f : records)
        {
            if (f.size() < 10 || f[0].empty())
            {
                skipped++;
                continue;
            }
            try
            {
                auto customer = make_unique<Customer>(f[0], f[1], f[2], f[3], f[4], "0000");
                customer->hashedPin = f[5];
                customer->registrationDate = f[6];
                customer->isActive = parseBool(f[7], true);
                customer->failedLoginAttempts = parseInt(f[8]);
                customer->isLocked = parseBool(f[9]);
                customers[f[0]] = std::move(customer);
            }
            catch (const exception &)
            {
                skipped++;
            }
        }
    }

    if (fileManager.readRecords("accounts.dat", records))
    {
        for (const auto &f : records)
        {
            AccountType type;
            if (f.size() < 10 || f[0].empty() || !accountTypeFromString(f[1], type))
            {
                skipped++;
                continue;
            }
            unique_ptr<Account> account;
            if (type == AccountType::SAVINGS)
            {
                auto savings = make_unique<SavingsAccount>(f[0], f[2], 0.0, parseDouble(f[6], savingsInterestRate),
                                                           parseDouble(f[7], minSavingsBalance), 6);
                savings->setWithdrawalCount(parseInt(f[8]));
                savings->setMaxWithdrawals(parseInt(f[9], maxSavingsWithdrawalsPerMonth));
                account = std::move(savings);
            }
            else
            {
                auto checking = make_unique<CheckingAccount>(f[0], f[2], 0.0, parseDouble(f[6], defaultOverdraftLimit),
                                                             parseDouble(f[7], checkingTransactionFee),
                                                             parseInt(f[8], freeCheckingTransactions));
                checking->setTransactionCount(parseInt(f[9]));
                account = std::move(checking);
            }
            account->updateBalance(parseDouble(f[3]));
            account->setCreationDate(f[4]);
            account->setActive(parseBool(f[5], true));
            auto ownerIt = customers.find(f[2]);
            registerAccount(std::move(account), ownerIt != customers.end() ? ownerIt->second.get() : nullptr);
        }
    }

    if (fileManager.readRecords("loans.dat", records))
    {
        for (const auto &f : records)
        {
            if (f.size() < 13 || f[0].empty())
            {
                skipped++;
                continue;
            }
            auto loan = make_unique<Loan>(f[0], f[1], f[2], parseDouble(f[3]), parseDouble(f[4]), parseInt(f[5], 1));
            loan->restoreState(parseDouble(f[7]), parseDouble(f[8]), parseDouble(f[9]), parseInt(f[10]), f[11],
                               Loan::statusFromString(f[12]), parseDouble(f[6]));
            loans[f[0]] = std::move(loan);
        }
    }

    transactionManager->loadTransactionHistory();
    syncCountersWithData();

    if (skipped > 0)
    {
        fileManager.logError("Skipped " + to_string(skipped) + " malformed record(s) while loading data");
        lastError = "Skipped " + to_string(skipped) + " malformed record(s) while loading; see logs/errors.log.";
    }
    fileManager.logSystem("Data loaded: " + to_string(customers.size()) + " customers, " +
                          to_string(accounts.size()) + " accounts, " + to_string(loans.size()) + " loans, " +
                          to_string(transactionManager->getTotalTransactionCount()) + " transactions");
    return true;
}

void Bank::setAutoSave(bool enabled)
{
    autoSave = enabled;
}

FileManager &Bank::getFileManager()
{
    return fileManager;
}

// =============================================================================
// SYSTEM VALIDATION
// =============================================================================

vector<string> Bank::findIntegrityIssues() const
{
    vector<string> issues;
    for (const auto &pair : accounts)
    {
        const Account *account = pair.second.get();
        const string &number = pair.first;
        auto ownerIt = customers.find(account->getCustomerId());
        if (ownerIt == customers.end())
        {
            issues.push_back("Account " + number + " belongs to missing customer " + account->getCustomerId());
        }
        else if (!ownerIt->second->hasAccount(number))
        {
            issues.push_back("Customer " + account->getCustomerId() + " is not linked to account " + number);
        }
        if (ownerOf(number) != account->getCustomerId())
            issues.push_back("Owner index for account " + number + " is out of date");
        if (account->getAccountType() == AccountType::SAVINGS && account->getBalance() < 0)
            issues.push_back("Savings account " + number + " has a negative balance");
        if (const CheckingAccount *checking = dynamic_cast<const CheckingAccount *>(account))
        {
            if (checking->getBalance() < -checking->getOverdraftLimit() - 0.005)
                issues.push_back("Checking account " + number + " is beyond its overdraft limit");
        }
        const Transaction *latest = transactionManager->getLatestAccountTransaction(number);
        if (latest && std::abs(latest->getBalanceAfter() - account->getBalance()) > 0.005)
            issues.push_back("Account " + number + " balance " + formatCurrency(account->getBalance()) +
                             " does not match its last transaction (" + formatCurrency(latest->getBalanceAfter()) + ")");
    }
    for (const auto &pair : accountCustomerMap)
    {
        if (accounts.find(pair.first) == accounts.end())
            issues.push_back("Owner index references closed account " + pair.first);
    }
    for (const auto &pair : customers)
    {
        for (const Account *account : pair.second->getAllAccounts())
        {
            auto it = accounts.find(account->getAccountNumber());
            if (it == accounts.end() || it->second.get() != account)
                issues.push_back("Customer " + pair.first + " links to unknown account " + account->getAccountNumber());
        }
    }
    for (const auto &pair : loans)
    {
        const Loan *loan = pair.second.get();
        if (loan->isActive() && customers.find(loan->getCustomerId()) == customers.end())
            issues.push_back("Loan " + pair.first + " belongs to missing customer " + loan->getCustomerId());
        if (loan->isActive() && accounts.find(loan->getLinkedAccountNumber()) == accounts.end())
            issues.push_back("Active loan " + pair.first + " is linked to missing account " + loan->getLinkedAccountNumber());
    }
    return issues;
}

bool Bank::validateSystemIntegrity() const
{
    return findIntegrityIssues().empty();
}

// Repair what can be repaired safely. Money is never moved automatically;
// balance mismatches stay in the issue list for manual review.
int Bank::fixDataInconsistencies()
{
    int fixes = 0;

    // Rebuild the owner index from the accounts themselves
    map<string, string> rebuilt;
    for (const auto &pair : accounts)
        rebuilt[pair.first] = pair.second->getCustomerId();
    if (rebuilt != accountCustomerMap)
    {
        accountCustomerMap = rebuilt;
        fixes++;
    }

    // Relink every customer's account list
    for (auto &pair : customers)
    {
        Customer *customer = pair.second.get();
        vector<Account *> expected;
        for (const auto &accountPair : accounts)
            if (accountPair.second->getCustomerId() == pair.first)
                expected.push_back(accountPair.second.get());
        if (customer->accounts != expected)
        {
            customer->accounts = expected;
            fixes++;
        }
    }

    // Freeze accounts whose owner no longer exists so nobody can move their money
    for (auto &pair : accounts)
    {
        if (customers.find(pair.second->getCustomerId()) == customers.end() && pair.second->isAccountActive())
        {
            pair.second->setActive(false);
            fileManager.logError("Froze orphaned account " + pair.first);
            fixes++;
        }
    }

    transactionManager->reindexTransactions();
    syncCountersWithData();
    if (fixes > 0)
    {
        fileManager.logSystem("Applied " + to_string(fixes) + " data repair(s)");
        persist();
    }
    return fixes;
}

// =============================================================================
// GETTERS
// =============================================================================

string Bank::getBankName() const
{
    return bankName;
}

string Bank::getBankCode() const
{
    return bankCode;
}

int Bank::getTotalCustomers() const
{
    return static_cast<int>(customers.size());
}

int Bank::getTotalAccounts() const
{
    return static_cast<int>(accounts.size());
}

double Bank::getTotalSystemBalance() const
{
    double total = 0.0;
    for (const auto &pair : accounts)
    {
        total += pair.second->getBalance();
    }
    return roundMoney(total);
}

int Bank::getTotalTransactions() const
{
    return transactionManager->getTotalTransactionCount();
}

int Bank::getSavingsAccountCount() const
{
    return static_cast<int>(count_if(accounts.begin(), accounts.end(), [](const auto &pair)
                                     { return pair.second->getAccountType() == AccountType::SAVINGS; }));
}

int Bank::getCheckingAccountCount() const
{
    return static_cast<int>(count_if(accounts.begin(), accounts.end(), [](const auto &pair)
                                     { return pair.second->getAccountType() == AccountType::CHECKING; }));
}

int Bank::getTotalActiveAccounts() const
{
    return static_cast<int>(count_if(accounts.begin(), accounts.end(), [](const auto &pair)
                                     { return pair.second->isAccountActive(); }));
}

int Bank::getTotalInactiveAccounts() const
{
    return getTotalAccounts() - getTotalActiveAccounts();
}

int Bank::getActiveCustomerCount() const
{
    return static_cast<int>(count_if(customers.begin(), customers.end(), [](const auto &pair)
                                     { return pair.second->isCustomerActive(); }));
}

int Bank::getInactiveCustomerCount() const
{
    return getTotalCustomers() - getActiveCustomerCount();
}

// =============================================================================
// ID GENERATION
// =============================================================================

string Bank::generateCustomerId()
{
    string id;
    do
    {
        stringstream ss;
        ss << "CUST" << setw(6) << setfill('0') << nextCustomerNumber++;
        id = ss.str();
    } while (customers.count(id) > 0);
    return id;
}

string Bank::generateSavingsAccountNumber()
{
    string number;
    do
    {
        stringstream ss;
        ss << "SAV" << setw(8) << setfill('0') << nextSavingsAccountNumber++;
        number = ss.str();
    } while (accounts.count(number) > 0);
    return number;
}

string Bank::generateCheckingAccountNumber()
{
    string number;
    do
    {
        stringstream ss;
        ss << "CHK" << setw(8) << setfill('0') << nextCheckingAccountNumber++;
        number = ss.str();
    } while (accounts.count(number) > 0);
    return number;
}

string Bank::generateLoanId()
{
    string id;
    do
    {
        stringstream ss;
        ss << "LN" << setw(6) << setfill('0') << nextLoanNumber++;
        id = ss.str();
    } while (loans.count(id) > 0);
    return id;
}
