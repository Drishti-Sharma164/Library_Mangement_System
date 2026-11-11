#include "Transaction.h"
#include <iostream>
#include "Utils.h"

Transaction::Transaction() : transactionId(0), studentId(0), bookId(0), returnDate("NULL") {}

Transaction::Transaction(int tid, int sid, int bid, const std::string& issue,
                         const std::string& ret)
    : transactionId(tid), studentId(sid), bookId(bid), issueDate(issue), returnDate(ret) {}

long Transaction::issueDays() const {
    long d = 0;
    Utils::parseDate(issueDate, d);
    return d;
}

long Transaction::dueDays() const { return issueDays() + Config::LOAN_DAYS; }

std::string Transaction::dueDate() const { return Utils::formatDate(dueDays()); }

// FINE RULE: every day after the 14-day period costs Rs. 2.
//   daysKept = returned - issued
//   daysLate = daysKept - 14       (never below 0)
//   fine     = daysLate * 2
int Transaction::computeFine(long issued, long returned) {
    long late = (returned - issued) - Config::LOAN_DAYS;
    return late > 0 ? (int)(late * Config::FINE_PER_DAY) : 0;
}

int Transaction::fineAsOf(long asOf) const {
    long end = asOf;
    if (isReturned()) Utils::parseDate(returnDate, end);
    return computeFine(issueDays(), end);
}

int Transaction::daysLateAsOf(long asOf) const {
    long end = asOf;
    if (isReturned()) Utils::parseDate(returnDate, end);
    long late = end - dueDays();
    return late > 0 ? (int)late : 0;
}

bool Transaction::isOverdueAsOf(long asOf) const { return !isReturned() && asOf > dueDays(); }

void Transaction::display() const {
    std::cout << "Txn " << transactionId << " | student " << studentId << " | book " << bookId
              << " | issued " << issueDate << " | due " << dueDate() << " | returned "
              << returnDate << "\n";
}

std::string Transaction::toRecord() const {
    return std::to_string(transactionId) + "|" + std::to_string(studentId) + "|" +
           std::to_string(bookId) + "|" + issueDate + "|" + returnDate;
}

bool Transaction::fromRecord(const std::string& line, Transaction& out) {
    std::vector<std::string> p = Utils::split(line, '|');
    int tid, sid, bid;
    long d;
    if (p.size() != 5 || !Utils::toInt(p[0], tid) || !Utils::toInt(p[1], sid) ||
        !Utils::toInt(p[2], bid))
        return false;
    std::string issue = Utils::trim(p[3]), ret = Utils::trim(p[4]);
    if (!Utils::parseDate(issue, d)) return false;
    if (ret != "NULL" && !Utils::parseDate(ret, d)) return false;
    out = Transaction(tid, sid, bid, issue, ret);
    return true;
}
