#ifndef LOAN_H
#define LOAN_H

#include <string>

using namespace std;

// Loan status enumeration
enum class LoanStatus {
    ACTIVE,
    PAID_OFF,
    DEFAULTED
};

// An amortised loan disbursed into one of the customer's accounts.
// Interest accrues monthly on the outstanding balance; payments reduce it.
class Loan {
private:
    string loanId;
    string customerId;
    string linkedAccountNumber;     // Disbursement and repayment account
    double principal;
    double annualInterestRate;      // e.g. 0.08 for 8%
    int termMonths;
    double monthlyPayment;          // Fixed EMI
    double outstandingBalance;
    double totalPaid;
    double totalInterestAccrued;
    int paymentsMade;
    string startDate;
    LoanStatus status;

public:
    // Constructor
    Loan(string id, string custId, string accountNumber, double amount, double annualRate, int months);

    // Standard amortisation formula: P * r / (1 - (1 + r)^-n), r = monthly rate
    static double calculateMonthlyPayment(double amount, double annualRate, int months);

    // Getters
    string getLoanId() const;
    string getCustomerId() const;
    string getLinkedAccountNumber() const;
    double getPrincipal() const;
    double getAnnualInterestRate() const;
    int getTermMonths() const;
    double getMonthlyPayment() const;
    double getOutstandingBalance() const;
    double getTotalPaid() const;
    double getTotalInterestAccrued() const;
    int getPaymentsMade() const;
    string getStartDate() const;
    LoanStatus getStatus() const;
    string getStatusString() const;
    bool isActive() const;

    // Setters (used when loading saved data and by account maintenance)
    void setLinkedAccountNumber(string accountNumber);
    void setCustomerId(string custId);
    void restoreState(double outstanding, double paid, double interestAccrued, int payments,
                      string start, LoanStatus loanStatus, double emi);
    void setStatus(LoanStatus newStatus);

    // Loan operations
    double accrueMonthlyInterest();     // Adds one month of interest, returns the amount
    double makePayment(double amount);  // Applies up to the outstanding balance, returns amount applied

    // Status string conversion
    static string statusToString(LoanStatus loanStatus);
    static LoanStatus statusFromString(const string& name);

    // Display
    void displayLoanInfo() const;
};

#endif // LOAN_H
