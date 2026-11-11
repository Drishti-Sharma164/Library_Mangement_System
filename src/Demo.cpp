// Prototype demonstration: runs the core logic on a throw-away in-memory library
// with FIXED dates, so the results are the same every time it is run.
#include "Demo.h"
#include <filesystem>
#include <iostream>
#include "Library.h"
#include "Utils.h"

using namespace std;

namespace {
int passed = 0, failed = 0;

void check(const string& name, bool ok, const string& detail = "") {
    cout << (ok ? "  [PASS] " : "  [FAIL] ") << name;
    if (!detail.empty()) cout << "  -> " << detail;
    cout << "\n";
    ok ? ++passed : ++failed;
}
long day(const string& s) { long d = 0; Utils::parseDate(s, d); return d; }
}  // namespace

int runDemo() {
    passed = failed = 0;
    cout << "\n================ PROTOTYPE SELF-TEST (Milestone 3) ================\n";
    cout << "Rules under test: " << Config::LOAN_DAYS << " days, max " << Config::MAX_BOOKS
         << " books, Rs. " << Config::FINE_PER_DAY << "/day fine\n";

    Library lib("demo_unused", false);  // memory only: no files are touched
    int b1 = lib.addBook("Clean Code", "Robert C. Martin", "Programming");
    int b2 = lib.addBook("Introduction to Algorithms", "Thomas H. Cormen", "Algorithms");
    int b3 = lib.addBook("Let Us C", "Yashavant Kanetkar", "Programming");
    int b4 = lib.addBook("Wings of Fire", "A. P. J. Abdul Kalam", "Biography");
    int b5 = lib.addBook("Discrete Mathematics", "Kenneth H. Rosen", "Mathematics");
    int s1 = lib.registerStudent("Asha Verma", "9876500011");
    int s2 = lib.registerStudent("Rohan Mehta", "9876500022");
    int fine = 0;
    Result r;

    cout << "\n--- 1. Date handling ---\n";
    long d1 = day("2028-02-28"), d2 = day("2028-03-01");
    check("Leap year: 2028-02-28 -> 2028-03-01 is 2 days", d2 - d1 == 2);
    check("Normal year: 2027-02-28 -> 2027-03-01 is 1 day", day("2027-03-01") - day("2027-02-28") == 1);
    long tmp;
    check("Impossible date 2026-02-30 rejected", !Utils::parseDate("2026-02-30", tmp));
    check("Bad format 20-09-2026 rejected", !Utils::parseDate("20-09-2026", tmp));

    cout << "\n--- 2. Issue logic ---\n";
    r = lib.issueBook(s1, b1, "2026-09-20");
    check("Issue Clean Code to Asha", r.ok, r.msg);
    check("Book becomes 'Issued'", !lib.findBook(b1)->isAvailable());
    check("Student record shows the book", lib.findStudent(s1)->hasBook(b1));
    r = lib.issueBook(s2, b1, "2026-09-21");
    check("Same book cannot be issued twice", !r.ok, r.msg);
    r = lib.issueBook(999, b2, "2026-09-21");
    check("Unregistered student rejected", !r.ok, r.msg);
    r = lib.issueBook(s2, 999, "2026-09-21");
    check("Unknown book rejected", !r.ok, r.msg);
    r = lib.issueBook(s2, b2, "2026-13-01");
    check("Invalid issue date rejected", !r.ok, r.msg);

    cout << "\n--- 3. Return + fine calculation ---\n";
    r = lib.returnBook(s1, b1, "2026-10-02", fine);
    check("Return after 12 days -> fine Rs. 0", r.ok && fine == 0, r.msg);
    check("Book is 'Available' again", lib.findBook(b1)->isAvailable());
    check("Student no longer holds the book", !lib.findStudent(s1)->hasBook(b1));

    lib.issueBook(s1, b2, "2026-09-20");
    r = lib.returnBook(s1, b2, "2026-10-04", fine);  // exactly 14 days
    check("Return on day 14 (last allowed day) -> Rs. 0", r.ok && fine == 0, r.msg);

    lib.issueBook(s1, b2, "2026-09-20");
    r = lib.returnBook(s1, b2, "2026-10-05", fine);  // 15 days
    check("Return on day 15 (1 day late) -> Rs. 2", r.ok && fine == 2, r.msg);

    lib.issueBook(s1, b3, "2026-09-20");
    r = lib.returnBook(s1, b3, "2026-10-10", fine);  // 20 days
    check("Return after 20 days (6 late) -> Rs. 12", r.ok && fine == 12, r.msg);

    lib.issueBook(s2, b4, "2026-01-01");
    r = lib.returnBook(s2, b4, "2026-12-31", fine);  // 364 days
    check("Very late: 364 days (350 late) -> Rs. 700", r.ok && fine == 700, r.msg);

    lib.issueBook(s1, b5, "2026-09-20");
    r = lib.returnBook(s2, b5, "2026-09-25", fine);
    check("Wrong student cannot return the book", !r.ok, r.msg);
    r = lib.returnBook(s1, b5, "2026-09-19", fine);
    check("Return date before issue date rejected", !r.ok, r.msg);
    r = lib.returnBook(s1, b5, "2026-09-25", fine);
    check("Correct return accepted", r.ok, r.msg);
    r = lib.returnBook(s1, b5, "2026-09-26", fine);
    check("Returning twice rejected", !r.ok, r.msg);

    cout << "\n--- 4. Borrow limit (max 3) ---\n";
    r = lib.issueBook(s1, b1, "2026-11-01");
    check("1st book", r.ok);
    r = lib.issueBook(s1, b2, "2026-11-01");
    check("2nd book", r.ok);
    r = lib.issueBook(s1, b3, "2026-11-01");
    check("3rd book", r.ok);
    r = lib.issueBook(s1, b4, "2026-11-01");
    check("4th book blocked by the limit", !r.ok, r.msg);
    r = lib.removeBook(b1);
    check("Cannot remove a book that is issued", !r.ok, r.msg);
    r = lib.deleteStudent(s1);
    check("Cannot delete a student who holds books", !r.ok, r.msg);

    cout << "\n--- 5. Overdue detection (as of 2026-11-20) ---\n";
    auto od = lib.overdueTransactions(day("2026-11-20"));
    check("3 books issued 2026-11-01 are overdue on 2026-11-20 (due 2026-11-15)", od.size() == 3,
          to_string(od.size()) + " overdue");
    check("Not overdue on the due date itself (2026-11-15)",
          lib.overdueTransactions(day("2026-11-15")).empty());
    check("Accrued fine on 2026-11-20 = 5 days x Rs. 2 = Rs. 10",
          od.size() == 3 && od[0]->fineAsOf(day("2026-11-20")) == 10);
    cout << "\n  Overdue report as of 2026-11-20:\n";
    lib.showOverdueBooks(day("2026-11-20"));

    // put things back for the search tests
    lib.returnBook(s1, b1, "2026-11-05", fine);
    lib.returnBook(s1, b2, "2026-11-05", fine);
    lib.returnBook(s1, b3, "2026-11-05", fine);
    lib.issueBook(s2, b3, "2026-11-06");  // Let Us C is out; the rest are in

    cout << "\n--- 6. Search and filter ---\n";
    using SF = SearchField;
    using AF = AvailFilter;
    check("Title search 'clean' (any case) finds 1", lib.searchBooks("CLEAN", SF::Title).size() == 1);
    check("Author search 'kalam' finds 1", lib.searchBooks("kalam", SF::Author).size() == 1);
    check("Category search 'programming' finds 2", lib.searchBooks("programming", SF::Category).size() == 2);
    check("Book-ID search finds exactly that book", lib.searchBooks(to_string(b2), SF::Id).size() == 1);
    check("'Everything' search 'math' finds Discrete Mathematics", lib.searchBooks("math").size() == 1);
    check("Empty word + 'Available only' lists 4 of 5 books", lib.searchBooks("", SF::All, "", AF::AvailableOnly).size() == 4);
    check("'Issued only' lists the 1 issued book", lib.searchBooks("", SF::All, "", AF::IssuedOnly).size() == 1);
    check("Category filter 'Programming' + Available -> 1 (Let Us C is out)",
          lib.searchBooks("", SF::All, "Programming", AF::AvailableOnly).size() == 1);
    check("Word 'let' + Programming + Issued -> Let Us C", lib.searchBooks("let", SF::Title, "programming", AF::IssuedOnly).size() == 1);
    check("No match returns an empty list", lib.searchBooks("zzzz").empty());
    cout << "\n  Example: Programming books, all statuses:\n";
    lib.printBooks(lib.searchBooks("", SF::All, "Programming", AF::Any));

    cout << "\n--- 7. File save / load round trip ---\n";
    const string tmpFolder = "demo_data_tmp";
    {
        Library a(tmpFolder, true);
        int x = a.addBook("Test | Pipe Title", "Some Author", "Testing");  // '|' must be cleaned
        int y = a.registerStudent("Test Student", "9999999999");
        a.issueBook(y, x, "2026-09-20");
    }
    {
        Library b(tmpFolder, true);
        b.load();
        check("1 book, 1 student, 1 transaction reloaded",
              b.bookCount() == 1 && b.studentCount() == 1 && b.transactionCount() == 1);
        const Book* bk = b.findBook(101);
        check("Pipe character in title was cleaned", bk && bk->getTitle() == "Test / Pipe Title");
        check("Issued state restored after reload", bk && !bk->isAvailable() && b.findStudent(201)->hasBook(101));
        int f = 0;
        r = b.returnBook(201, 101, "2026-10-10", f);
        check("Return after reload works (6 days late -> Rs. 12)", r.ok && f == 12, r.msg);
    }
    error_code ec;
    filesystem::remove_all(tmpFolder, ec);

    cout << "\n===================================================================\n";
    cout << "  RESULT: " << passed << " passed, " << failed << " failed, " << (passed + failed)
         << " checks in total.\n";
    cout << "===================================================================\n";
    return failed == 0 ? 0 : 1;
}
