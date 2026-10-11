# Banking System in C++

A console banking application written in modern C++17. Customers can register, open savings and checking accounts, move money, take out loans and download statements. Administrators get reports, account controls, month-end processing, backups and integrity checks. All data is kept in plain-text files and survives restarts.

## Features

**Customers**
- Self-registration with validated name, phone, email and a 4–6 digit PIN (stored as a salted SHA-256 hash)
- Savings accounts: minimum balance, monthly interest, limited withdrawals per month
- Checking accounts: overdraft, a number of free transactions each month, then a fee per transaction
- Deposits, withdrawals, transfers between your own accounts or to any other account
- Transaction history, statements for a date range, CSV export
- Loans: monthly payment (EMI) shown before you accept, money paid into your account, payments taken from it, interest added each month
- Update contact details, change PIN, close accounts (the remaining balance is moved or paid out)

**Administrators**
- System statistics, customer/account/loan listings, customer drill-down
- Daily and monthly reports (activity by type, growth, top customers)
- Freeze/unfreeze accounts, activate/deactivate customers, unlock customers and reset PINs
- Close accounts, delete customers, transfer account ownership, convert account type, set overdraft limits
- Reverse transactions (transfers reverse both legs and the money goes back)
- Month-end processing: savings interest, checking fees, counter resets, loan interest (protected against running twice in a month)
- Settings that can be changed from the menu and are written back to `config/bank_config.txt`
- System maintenance: integrity check with automatic repair, log cleanup, backup
- Timestamped backups and restore (the current state is backed up before a restore)

**Security and reliability**
- Customer lockout after `MAX_LOGIN_ATTEMPTS` failed logins (kept across restarts until an admin unlocks it)
- Session timeout after a period of inactivity; PINs and passwords are not echoed on screen
- Daily cash withdrawal limit and per-transfer limit
- Every change to a balance is recorded as a transaction, with balance before/after and the session ID
- Data is saved after every change; files are written to a temp file and then renamed, so a crash can't leave a half-written file
- Automatic backup at start-up once `BACKUP_INTERVAL` hours have passed; logs rotate when they reach `MAX_LOG_SIZE`

## Building

Requires a C++17 compiler (GCC 8+, Clang 7+, Apple Clang, MSVC 2019+).

```bash
make            # build build/bin/banking_system
make run        # build and start the interactive application
make demo       # run a scripted walkthrough in a temporary directory
make test       # build and run the test suite
make debug      # build with AddressSanitizer + UBSan
make release    # optimised build
```

Or with CMake:

```bash
cmake -S . -B build/cmake
cmake --build build/cmake
ctest --test-dir build/cmake
```

## Running

Start the program from the project root so it finds `config/`:

```bash
./build/bin/banking_system
```

| Option | Default | Purpose |
| --- | --- | --- |
| `--data-dir DIR` | `data` | Customer, account, loan and transaction files |
| `--backup-dir DIR` | `backup` | Timestamped backups |
| `--log-dir DIR` | `logs` | `system.log`, `transactions.log`, `errors.log` |
| `--config-dir DIR` | `config` | `bank_config.txt` and `admin_settings.txt` |
| `--demo` | | Scripted walkthrough; uses a temporary directory and leaves your data untouched |

**First run:** choose *Register as a New Customer*, note the Customer ID you are given (e.g. `CUST000001`), then log in with it and your PIN.

**Admin login:** username `admin`, password `password`. The application reminds you to change it; *Change Admin Password* writes the new SHA-256 hash to `config/admin_settings.txt`. To set a password by hand: `printf '%s' 'NewPass1!' | shasum -a 256`.

## Configuration

`config/bank_config.txt`

| Key | Meaning |
| --- | --- |
| `MIN_SAVINGS_BALANCE`, `MIN_CHECKING_BALANCE` | Minimum opening deposit; savings accounts must also stay at or above their minimum |
| `SAVINGS_INTEREST_RATE` | Annual rate as a fraction (`0.04` = 4%), credited monthly |
| `LOAN_INTEREST_RATE` | Annual rate for new loans |
| `MAX_DAILY_WITHDRAWAL` | Cash withdrawal limit per account per day |
| `MAX_TRANSFER_AMOUNT` | Limit per transfer |
| `MAX_WITHDRAWAL_PER_MONTH` | Savings withdrawals allowed per month |
| `OVERDRAFT_LIMIT` | Default overdraft for new checking accounts |
| `TRANSACTION_FEE` | Checking fee per transaction beyond the free allowance |
| `SESSION_TIMEOUT` | Customer inactivity timeout (seconds) |
| `MAX_LOGIN_ATTEMPTS` | Failed logins before lockout |
| `BACKUP_INTERVAL` | Hours between automatic start-up backups |

Optional keys: `FREE_CHECKING_TRANSACTIONS` (default 10), `MAX_LOAN_AMOUNT` (default 50000), `MAX_ACTIVE_LOANS` (default 3), `BANK_NAME`, `BANK_CODE`.

`config/admin_settings.txt` holds the admin username and password hash, the password policy (`PASSWORD_MIN_LENGTH`, `REQUIRE_SPECIAL_CHARS`), the admin `SESSION_TIMEOUT`, `AUTO_BACKUP`, `LOG_RETENTION_DAYS` and `MAX_LOG_SIZE`.

## Project Structure

```
Banking-System-On-Cpp/
├── src/
│   ├── headers/
│   │   ├── Account.h          # Account base class, SavingsAccount, CheckingAccount
│   │   ├── Customer.h         # Customer profile, PIN, lockout, account links
│   │   ├── Transaction.h      # Transaction record and TransactionManager (ledger)
│   │   ├── Loan.h             # Loan with EMI calculation and repayments
│   │   ├── Bank.h             # Business rules; owns customers, accounts and loans
│   │   ├── FileManager.h      # Record files, config files, backups, logs
│   │   ├── Login.h            # Authentication, lockout, sessions
│   │   ├── UI.h               # Menu-driven console interface
│   │   └── Utilities.h        # Validation, dates, money formatting, SHA-256
│   ├── implementation/        # Matching .cpp files
│   └── main.cpp               # Start-up, command-line options, demo mode
├── tests/test_main.cpp        # Unit and integration tests
├── config/                    # bank_config.txt, admin_settings.txt
├── data/                      # Created at runtime (*.dat files)
├── backup/                    # Created at runtime (one folder per backup)
├── logs/                      # Created at runtime
├── Makefile
└── CMakeLists.txt
```

## Design Notes

- **Layering.** `Account`, `Customer`, `Loan` and `Transaction` are plain model classes and never print. `Bank` enforces the rules and reports failures through `getLastError()`. `UI` does all the input and output. That keeps the core testable and reusable.
- **Ownership.** `Bank` owns every account through `std::unique_ptr`; a `Customer` only keeps links (non-owning pointers) to its accounts. This fixes the earlier double-free when closing accounts and the dangling pointer after ownership transfers.
- **The ledger as source of truth.** Every change to a balance goes through `TransactionManager`, so the integrity check can confirm that each account's balance matches its latest transaction. Transfers create linked `TRANSFER_OUT`/`TRANSFER_IN` pairs. Reversals add a new correcting transaction instead of editing history.
- **IDs.** `CUST000001`, `SAV00000001`, `CHK00000001`, `LN000001`, `TXN<yyyymmdd><sequence>`. Counters are rebuilt from the saved data and the transaction log on load, so numbers are never reused, including numbers of closed accounts.
- **File format.** One `|`-separated record per line, with `\`-escaping so names, addresses and descriptions can contain any character. Lines starting with `#` are comments.
- **Money.** Amounts are rounded to whole cents at every step to avoid floating-point drift.
