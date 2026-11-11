#include "Library.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include "UI.h"

Library::Library(const std::string& dataFolder, bool isPersistent)
    : librarian(), fileManager(dataFolder), persistent(isPersistent) {}

// ======================= files =======================
bool Library::load() {
    if (!persistent) return true;
    fileManager.resetSkipped();
    books = fileManager.loadBooks();
    students = fileManager.loadStudents();
    transactions = fileManager.loadTransactions();
    reconcile();
    if (fileManager.getSkippedLines() > 0)
        UI::warning(std::to_string(fileManager.getSkippedLines()) +
                    " damaged line(s) in the data files were ignored.");
    return true;
}

bool Library::save() {
    if (!persistent) return true;
    return fileManager.saveBooks(books) && fileManager.saveStudents(students) &&
           fileManager.saveTransactions(transactions);
}

void Library::persist() {
    if (persistent && !save())
        UI::error("Could not save data files. Check the data/ folder.");
}

// Open transactions are the "truth": a book is Issued only if it has an open transaction,
// and a student holds a book only if it has an open transaction for that student.
void Library::reconcile() {
    for (auto& b : books) b.markReturned();
    for (auto& s : students) s.clearIssued();
    for (const auto& t : transactions) {
        if (t.isReturned()) continue;
        Book* b = findBookPtr(t.getBookId());
        Student* s = findStudentPtr(t.getStudentId());
        if (b) b->markIssued();
        if (s) s->addIssuedBook(t.getBookId());
    }
}

void Library::loadSampleData() {
    addBook("Clean Code", "Robert C. Martin", "Programming");
    addBook("The Pragmatic Programmer", "Andrew Hunt", "Programming");
    addBook("Let Us C", "Yashavant Kanetkar", "Programming");
    addBook("Introduction to Algorithms", "Thomas H. Cormen", "Algorithms");
    addBook("Data Structures Using C++", "D. S. Malik", "Data Structures");
    addBook("Discrete Mathematics", "Kenneth H. Rosen", "Mathematics");
    addBook("Wings of Fire", "A. P. J. Abdul Kalam", "Biography");
    registerStudent("Asha Verma", "9876500011");
    registerStudent("Rohan Mehta", "9876500022");
    registerStudent("Priya Singh", "9876500033");
}

// ======================= id helpers =======================
int Library::nextBookId() const {
    int m = 100;
    for (const auto& b : books) m = std::max(m, b.getId());
    for (const auto& t : transactions) m = std::max(m, t.getBookId());
    return m + 1;
}
int Library::nextStudentId() const {
    int m = 200;
    for (const auto& s : students) m = std::max(m, s.getId());
    for (const auto& t : transactions) m = std::max(m, t.getStudentId());
    return m + 1;
}
int Library::nextTransactionId() const {
    int m = 1000;
    for (const auto& t : transactions) m = std::max(m, t.getId());
    return m + 1;
}

Book* Library::findBookPtr(int id) {
    for (auto& b : books) if (b.getId() == id) return &b;
    return nullptr;
}
Student* Library::findStudentPtr(int id) {
    for (auto& s : students) if (s.getId() == id) return &s;
    return nullptr;
}
const Book* Library::findBook(int id) const {
    for (const auto& b : books) if (b.getId() == id) return &b;
    return nullptr;
}
const Student* Library::findStudent(int id) const {
    for (const auto& s : students) if (s.getId() == id) return &s;
    return nullptr;
}

// ======================= books =======================
int Library::addBook(const std::string& title, const std::string& author,
                     const std::string& category) {
    int id = nextBookId();
    books.push_back(Book(id, Utils::sanitize(title), Utils::sanitize(author),
                         Utils::sanitize(category)));
    persist();
    return id;
}

Result Library::updateBook(int id, const std::string& title, const std::string& author,
                           const std::string& category) {
    Book* b = findBookPtr(id);
    if (!b) return {false, "No book with ID " + std::to_string(id) + "."};
    if (!Utils::sanitize(title).empty()) b->setTitle(Utils::sanitize(title));
    if (!Utils::sanitize(author).empty()) b->setAuthor(Utils::sanitize(author));
    if (!Utils::sanitize(category).empty()) b->setCategory(Utils::sanitize(category));
    persist();
    return {true, "Book " + std::to_string(id) + " updated."};
}

Result Library::removeBook(int id) {
    auto it = std::find_if(books.begin(), books.end(),
                           [id](const Book& b) { return b.getId() == id; });
    if (it == books.end()) return {false, "No book with ID " + std::to_string(id) + "."};
    if (!it->isAvailable())
        return {false, "Book is currently issued. It can be removed after it is returned."};
    books.erase(it);
    persist();
    return {true, "Book removed."};
}

// ======================= students =======================
int Library::registerStudent(const std::string& name, const std::string& contact) {
    int id = nextStudentId();
    students.push_back(Student(id, Utils::sanitize(name), Utils::sanitize(contact)));
    persist();
    return id;
}

Result Library::updateStudent(int id, const std::string& name, const std::string& contact) {
    Student* s = findStudentPtr(id);
    if (!s) return {false, "No student with ID " + std::to_string(id) + "."};
    if (!Utils::sanitize(name).empty()) s->setName(Utils::sanitize(name));
    if (!Utils::sanitize(contact).empty()) s->setContact(Utils::sanitize(contact));
    persist();
    return {true, "Student " + std::to_string(id) + " updated."};
}

Result Library::deleteStudent(int id) {
    auto it = std::find_if(students.begin(), students.end(),
                           [id](const Student& s) { return s.getId() == id; });
    if (it == students.end()) return {false, "No student with ID " + std::to_string(id) + "."};
    if (it->issuedCount() > 0)
        return {false, "Student still holds " + std::to_string(it->issuedCount()) +
                           " book(s). Return them first."};
    students.erase(it);
    persist();
    return {true, "Student deleted."};
}

// ======================= issue / return =======================
// ISSUE steps:  1) student exists?  2) book exists?  3) book available?
//               4) student below the 3-book limit?  5) date valid?
//               6) create transaction, mark book issued, update student, save.
Result Library::issueBook(int studentId, int bookId, const std::string& date) {
    Student* s = findStudentPtr(studentId);
    if (!s) return {false, "Student " + std::to_string(studentId) + " is not registered."};
    Book* b = findBookPtr(bookId);
    if (!b) return {false, "Book " + std::to_string(bookId) + " does not exist."};
    if (!b->isAvailable()) return {false, "\"" + b->getTitle() + "\" is already issued."};
    if (s->issuedCount() >= Config::MAX_BOOKS)
        return {false, s->getName() + " already holds " + std::to_string(Config::MAX_BOOKS) +
                           " books (the limit). Return one first."};

    long issueDay = 0;
    std::string issueDate = Utils::trim(date);
    if (issueDate.empty()) issueDate = Utils::todayString();
    if (!Utils::parseDate(issueDate, issueDay))
        return {false, "Invalid date \"" + issueDate + "\". Use YYYY-MM-DD."};

    Transaction t(nextTransactionId(), studentId, bookId, issueDate);
    transactions.push_back(t);
    b->markIssued();
    s->addIssuedBook(bookId);
    persist();
    return {true, "Issued \"" + b->getTitle() + "\" to " + s->getName() + ". Transaction " +
                      std::to_string(t.getId()) + ". Issue date " + issueDate + ", due " +
                      t.dueDate() + "."};
}

// RETURN steps: 1) find the open transaction for this student + book
//               2) validate return date (not before issue date)
//               3) fine = max(0, daysKept - 14) * 2
//               4) close transaction, mark book available, update student, save.
Result Library::returnBook(int studentId, int bookId, const std::string& date, int& fineOut) {
    fineOut = 0;
    Transaction* open = nullptr;
    for (auto& t : transactions)
        if (!t.isReturned() && t.getBookId() == bookId && t.getStudentId() == studentId) {
            open = &t;
            break;
        }
    if (!open)
        return {false, "No open issue record found for student " + std::to_string(studentId) +
                           " and book " + std::to_string(bookId) + "."};

    std::string retDate = Utils::trim(date);
    if (retDate.empty()) retDate = Utils::todayString();
    long retDay = 0;
    if (!Utils::parseDate(retDate, retDay))
        return {false, "Invalid date \"" + retDate + "\". Use YYYY-MM-DD."};
    if (retDay < open->issueDays())
        return {false, "Return date cannot be before the issue date (" + open->getIssueDate() +
                           ")."};

    fineOut = Transaction::computeFine(open->issueDays(), retDay);
    int daysKept = (int)(retDay - open->issueDays());
    int daysLate = open->daysLateAsOf(retDay);
    open->setReturnDate(retDate);

    Book* b = findBookPtr(bookId);
    Student* s = findStudentPtr(studentId);
    if (b) b->markReturned();
    if (s) s->removeIssuedBook(bookId);
    persist();

    std::string title = b ? b->getTitle() : ("Book " + std::to_string(bookId));
    std::string msg = "Returned \"" + title + "\" on " + retDate + ". Kept " +
                      std::to_string(daysKept) + " day(s). ";
    if (fineOut > 0)
        msg += "LATE by " + std::to_string(daysLate) + " day(s). FINE = Rs. " +
               std::to_string(fineOut) + " (" + std::to_string(daysLate) + " x Rs. " +
               std::to_string(Config::FINE_PER_DAY) + ").";
    else
        msg += "Returned on time. No fine.";
    return {true, msg};
}

// ======================= search and filter =======================
std::vector<const Book*> Library::searchBooks(const std::string& keyword, SearchField field,
                                              const std::string& category,
                                              AvailFilter avail) const {
    std::vector<const Book*> out;
    const std::string kw = Utils::trim(keyword);
    const std::string cat = Utils::trim(category);
    int kwId = -1;
    const bool kwIsId = Utils::toInt(kw, kwId);

    for (const auto& b : books) {
        if (avail == AvailFilter::AvailableOnly && !b.isAvailable()) continue;
        if (avail == AvailFilter::IssuedOnly && b.isAvailable()) continue;
        if (!cat.empty() && !Utils::equalsIgnoreCase(b.getCategory(), cat)) continue;

        bool match = kw.empty();
        if (!match) {
            switch (field) {
                case SearchField::Title: match = Utils::containsIgnoreCase(b.getTitle(), kw); break;
                case SearchField::Author: match = Utils::containsIgnoreCase(b.getAuthor(), kw); break;
                case SearchField::Category: match = Utils::containsIgnoreCase(b.getCategory(), kw); break;
                case SearchField::Id: match = kwIsId && b.getId() == kwId; break;
                case SearchField::All:
                    match = Utils::containsIgnoreCase(b.getTitle(), kw) ||
                            Utils::containsIgnoreCase(b.getAuthor(), kw) ||
                            Utils::containsIgnoreCase(b.getCategory(), kw) ||
                            (kwIsId && b.getId() == kwId);
                    break;
            }
        }
        if (match) out.push_back(&b);
    }
    return out;
}

std::vector<std::string> Library::categories() const {
    std::vector<std::string> cats;
    for (const auto& b : books) {
        bool seen = false;
        for (const auto& c : cats)
            if (Utils::equalsIgnoreCase(c, b.getCategory())) { seen = true; break; }
        if (!seen) cats.push_back(b.getCategory());
    }
    std::sort(cats.begin(), cats.end());
    return cats;
}

std::vector<const Transaction*> Library::overdueTransactions(long asOf) const {
    std::vector<const Transaction*> out;
    for (const auto& t : transactions)
        if (t.isOverdueAsOf(asOf)) out.push_back(&t);
    return out;
}

// ======================= screens =======================
void Library::printBooks(const std::vector<const Book*>& list) const {
    if (list.empty()) { UI::info("No matching books found."); return; }
    Book::printHeader();
    for (const Book* b : list) b->display();
    UI::Table({{"ID", 5}, {"Title", 27}, {"Author", 22}, {"Category", 16}, {"Status", 9}})
        .footer(std::to_string(list.size()) + " book(s) shown.");
}

void Library::showAllBooks() const {
    std::vector<const Book*> all;
    for (const auto& b : books) all.push_back(&b);
    printBooks(all);
}

void Library::showStudents() const {
    if (students.empty()) { UI::info("No students registered."); return; }
    Student::printHeader();
    for (const auto& s : students) s.display();
    UI::Table({{"ID", 5}, {"Name", 26}, {"Contact", 15}, {"Books held", 10}})
        .footer(std::to_string(students.size()) + " student(s).");
}

void Library::printTransactions(const std::vector<const Transaction*>& list, long asOf) const {
    if (list.empty()) { UI::info("No records."); return; }
    UI::Table t({{"Txn", 5}, {"Student", 18}, {"Book", 22}, {"Issued", 10}, {"Due", 10},
                 {"Returned", 10}, {"Fine", 7}, {"Status", 8}});
    t.header();
    for (const Transaction* tr : list) {
        const Student* s = findStudent(tr->getStudentId());
        const Book* b = findBook(tr->getBookId());
        std::string sn = std::to_string(tr->getStudentId()) + ":" + (s ? s->getName() : "?");
        std::string bn = std::to_string(tr->getBookId()) + ":" + (b ? b->getTitle() : "?");
        bool overdue = tr->isOverdueAsOf(asOf);
        std::string status = tr->isReturned() ? "Returned" : (overdue ? "OVERDUE" : "Issued");
        UI::Color col = tr->isReturned() ? UI::GREEN : (overdue ? UI::RED : UI::YELLOW);
        t.row({std::to_string(tr->getId()), sn, bn, tr->getIssueDate(), tr->dueDate(),
               tr->getReturnDate(), "Rs." + std::to_string(tr->fineAsOf(asOf)), status},
              7, col);
    }
    t.footer(std::to_string(list.size()) + " record(s).  (Fine for books not yet returned is the " +
             "fine owed as of " + Utils::formatDate(asOf) + ".)");
}

void Library::showTransactions() const {
    std::vector<const Transaction*> all;
    for (const auto& t : transactions) all.push_back(&t);
    printTransactions(all, Utils::todayDays());
}

void Library::showIssuedBooks() const {
    std::vector<const Transaction*> open;
    for (const auto& t : transactions) if (!t.isReturned()) open.push_back(&t);
    printTransactions(open, Utils::todayDays());
}

void Library::showOverdueBooks(long asOf) const {
    printTransactions(overdueTransactions(asOf), asOf);
}

void Library::showStudentIssued(int studentId, long asOf) const {
    std::vector<const Transaction*> mine;
    for (const auto& t : transactions)
        if (!t.isReturned() && t.getStudentId() == studentId) mine.push_back(&t);
    printTransactions(mine, asOf);
}
