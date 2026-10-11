#include "Loan.h"
#include "Utilities.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

using namespace std;

// =============================================================================
// CONSTRUCTOR
// =============================================================================

Loan::Loan(string id, string custId, string accountNumber, double amount, double annualRate, int months) {
    loanId = id;
    customerId = custId;
    linkedAccountNumber = accountNumber;
    principal = roundMoney(max(0.0, amount));
    annualInterestRate = max(0.0, annualRate);
    termMonths = max(1, months);
    monthlyPayment = calculateMonthlyPayment(principal, annualInterestRate, termMonths);
    outstandingBalance = principal;
    totalPaid = 0.0;
    totalInterestAccrued = 0.0;
    paymentsMade = 0;
    startDate = getCurrentDate();
    status = principal > 0 ? LoanStatus::ACTIVE : LoanStatus::PAID_OFF;
}

// Standard amortisation formula: P * r / (1 - (1 + r)^-n), r = monthly rate
double Loan::calculateMonthlyPayment(double amount, double annualRate, int months) {
    if (amount <= 0 || months <= 0) return 0.0;
    double r = annualRate / 12.0;
    if (r <= 0) return roundMoney(amount / months);
    double payment = amount * r / (1.0 - pow(1.0 + r, -months));
    return roundMoney(payment);
}

// =============================================================================
// GETTERS
// =============================================================================

string Loan::getLoanId() const { return loanId; }
string Loan::getCustomerId() const { return customerId; }
string Loan::getLinkedAccountNumber() const { return linkedAccountNumber; }
double Loan::getPrincipal() const { return principal; }
double Loan::getAnnualInterestRate() const { return annualInterestRate; }
int Loan::getTermMonths() const { return termMonths; }
double Loan::getMonthlyPayment() const { return monthlyPayment; }
double Loan::getOutstandingBalance() const { return outstandingBalance; }
double Loan::getTotalPaid() const { return totalPaid; }
double Loan::getTotalInterestAccrued() const { return totalInterestAccrued; }
int Loan::getPaymentsMade() const { return paymentsMade; }
string Loan::getStartDate() const { return startDate; }
LoanStatus Loan::getStatus() const { return status; }
string Loan::getStatusString() const { return statusToString(status); }
bool Loan::isActive() const { return status == LoanStatus::ACTIVE; }

// =============================================================================
// SETTERS
// =============================================================================

void Loan::setLinkedAccountNumber(string accountNumber) {
    linkedAccountNumber = accountNumber;
}

void Loan::setCustomerId(string custId) {
    customerId = custId;
}

void Loan::restoreState(double outstanding, double paid, double interestAccrued, int payments,
                        string start, LoanStatus loanStatus, double emi) {
    outstandingBalance = roundMoney(max(0.0, outstanding));
    totalPaid = roundMoney(max(0.0, paid));
    totalInterestAccrued = roundMoney(max(0.0, interestAccrued));
    paymentsMade = max(0, payments);
    if (!start.empty()) startDate = start;
    status = loanStatus;
    if (emi > 0) monthlyPayment = emi;
}

void Loan::setStatus(LoanStatus newStatus) {
    status = newStatus;
}

// =============================================================================
// LOAN OPERATIONS
// =============================================================================

// Add one month of interest to the outstanding balance
double Loan::accrueMonthlyInterest() {
    if (status != LoanStatus::ACTIVE || outstandingBalance <= 0) return 0.0;
    double interest = roundMoney(outstandingBalance * annualInterestRate / 12.0);
    outstandingBalance = roundMoney(outstandingBalance + interest);
    totalInterestAccrued = roundMoney(totalInterestAccrued + interest);
    return interest;
}

// Apply a payment (capped at the outstanding balance); returns the amount applied
double Loan::makePayment(double amount) {
    if (status != LoanStatus::ACTIVE || amount <= 0) return 0.0;
    double applied = roundMoney(min(amount, outstandingBalance));
    outstandingBalance = roundMoney(outstandingBalance - applied);
    totalPaid = roundMoney(totalPaid + applied);
    paymentsMade++;
    if (outstandingBalance <= 0) {
        outstandingBalance = 0.0;
        status = LoanStatus::PAID_OFF;
    }
    return applied;
}

// =============================================================================
// STATUS CONVERSION
// =============================================================================

string Loan::statusToString(LoanStatus loanStatus) {
    switch (loanStatus) {
    case LoanStatus::ACTIVE:    return "ACTIVE";
    case LoanStatus::PAID_OFF:  return "PAID_OFF";
    case LoanStatus::DEFAULTED: return "DEFAULTED";
    default:                    return "UNKNOWN";
    }
}

LoanStatus Loan::statusFromString(const string& name) {
    string upper = toUpper(trim(name));
    if (upper == "PAID_OFF") return LoanStatus::PAID_OFF;
    if (upper == "DEFAULTED") return LoanStatus::DEFAULTED;
    return LoanStatus::ACTIVE;
}

// =============================================================================
// DISPLAY
// =============================================================================

void Loan::displayLoanInfo() const {
    cout << "Loan ID            : " << loanId << endl;
    cout << "Customer ID        : " << customerId << endl;
    cout << "Linked Account     : " << linkedAccountNumber << endl;
    cout << "Principal          : " << formatCurrency(principal) << endl;
    cout << "Interest Rate      : " << fixed << setprecision(2) << annualInterestRate * 100 << "% p.a." << endl;
    cout << "Term               : " << termMonths << " months" << endl;
    cout << "Monthly Payment    : " << formatCurrency(monthlyPayment) << endl;
    cout << "Outstanding        : " << formatCurrency(outstandingBalance) << endl;
    cout << "Total Paid         : " << formatCurrency(totalPaid) << " in " << paymentsMade << " payment(s)" << endl;
    cout << "Interest Accrued   : " << formatCurrency(totalInterestAccrued) << endl;
    cout << "Start Date         : " << startDate << endl;
    cout << "Status             : " << getStatusString() << endl;
}
