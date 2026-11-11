// Transaction.h - one borrowing / returning record (also does the fine maths)
#pragma once
#include <string>

class Transaction {
private:
    int transactionId;
    int studentId;
    int bookId;
    std::string issueDate;   // YYYY-MM-DD
    std::string returnDate;  // YYYY-MM-DD or "NULL" when not returned yet

public:
    Transaction();
    Transaction(int tid, int sid, int bid, const std::string& issueDate,
                const std::string& returnDate = "NULL");

    int getId() const { return transactionId; }
    int getStudentId() const { return studentId; }
    int getBookId() const { return bookId; }
    const std::string& getIssueDate() const { return issueDate; }
    const std::string& getReturnDate() const { return returnDate; }

    bool isReturned() const { return returnDate != "NULL"; }
    void setReturnDate(const std::string& date) { returnDate = date; }

    // ---- due date and fine calculation ----
    long issueDays() const;
    long dueDays() const;
    std::string dueDate() const;
    // fine if a book issued on day 'issued' is returned on day 'returned'
    static int computeFine(long issued, long returned);
    // fine for this record: uses the return date if returned, else 'asOf' (today)
    int fineAsOf(long asOf) const;
    int daysLateAsOf(long asOf) const;
    bool isOverdueAsOf(long asOf) const;

    void display() const;

    // transactions.txt line: transactionId|studentId|bookId|issueDate|returnDate
    std::string toRecord() const;
    static bool fromRecord(const std::string& line, Transaction& out);
};
