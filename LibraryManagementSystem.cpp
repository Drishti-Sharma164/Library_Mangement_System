// =====================================================================
// Library Management System - single-file edition (C++17)
// Build:  g++ -std=c++17 LibraryManagementSystem.cpp -o library
// Run  :  ./library        (self-test: ./library --demo)
// This file is the whole project (include/*.h and src/*.cpp) joined together.
// =====================================================================

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

// ############ include/Utils.h ############
// Utils.h - shared constants, helpers for dates, text and console input

// ---------- Library rules (from the Milestone 1 design) ----------
namespace Config {
const int LOAN_DAYS = 14;     // a book may be kept for 14 days
const int MAX_BOOKS = 3;      // a student may hold at most 3 books
const int FINE_PER_DAY = 2;   // late fine: Rs. 2 per day
}

// Simple success/failure answer with a message for the screen
struct Result {
    bool ok;
    std::string msg;
};

namespace Utils {

// ---------- text helpers ----------
inline std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace((unsigned char)s[b])) ++b;
    while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}
inline std::string toLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
inline bool containsIgnoreCase(const std::string& hay, const std::string& needle) {
    return toLower(hay).find(toLower(needle)) != std::string::npos;
}
inline bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}
// '|' is the file separator, so it must never appear inside a field
inline std::string sanitize(std::string s) {
    s = trim(s);
    for (auto& c : s) if (c == '|' || c == '\n' || c == '\r') c = '/';
    return s;
}
inline bool toInt(const std::string& s, int& out) {
    std::string t = trim(s);
    if (t.empty()) return false;
    try {
        size_t p = 0;
        long v = std::stol(t, &p);
        if (p != t.size() || v < 0 || v > 2000000000L) return false;
        out = (int)v;
        return true;
    } catch (...) { return false; }
}
// shorten text so it fits a table column
inline std::string fit(const std::string& s, size_t w) {
    if (s.size() <= w) return s;
    if (w <= 2) return s.substr(0, w);
    return s.substr(0, w - 2) + "..";
}

// ---------- password hashing (FNV-1a, salted) ----------
// Learning-grade: passwords are never stored as plain text.
inline std::string hashPassword(const std::string& pw) {
    uint64_t h = 1469598103934665603ULL;
    const std::string salted = "LMS-salt-2026:" + pw;
    for (unsigned char c : salted) { h ^= c; h *= 1099511628211ULL; }
    std::ostringstream o;
    o << std::hex << std::setw(16) << std::setfill('0') << h;
    return o.str();
}

// ---------- dates (no time-zone tricks: pure calendar arithmetic) ----------
// days since 1970-01-01 for a calendar date (Howard Hinnant's algorithm)
inline long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
inline void civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long yy = yoe + era * 400;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    y = (int)(yy + (m <= 2));
}
// "YYYY-MM-DD" -> day number. Rejects things like 2026-02-30.
inline bool parseDate(const std::string& text, long& days) {
    std::string s = trim(text);
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < 10; ++i)
        if (i != 4 && i != 7 && !std::isdigit((unsigned char)s[i])) return false;
    int y = std::stoi(s.substr(0, 4));
    int m = std::stoi(s.substr(5, 2));
    int d = std::stoi(s.substr(8, 2));
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    long z = daysFromCivil(y, m, d);
    int y2, m2, d2;
    civilFromDays(z, y2, m2, d2);
    if (y2 != y || m2 != m || d2 != d) return false;
    days = z;
    return true;
}
inline std::string formatDate(long days) {
    int y, m, d;
    civilFromDays(days, y, m, d);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", y, m, d);
    return buf;
}
inline long todayDays() {
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    return daysFromCivil(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
}
inline std::string todayString() { return formatDate(todayDays()); }

// ---------- console input ----------
inline std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) {  // input closed (Ctrl+D / end of file)
        std::cout << "\nInput closed. Data is already saved. Goodbye!\n";
        std::exit(0);
    }
    return s;
}
inline std::string readNonEmpty(const std::string& prompt) {
    while (true) {
        std::string s = sanitize(readLine(prompt));
        if (!s.empty()) return s;
        std::cout << "  This field cannot be empty.\n";
    }
}
inline int readInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string s = trim(readLine(prompt));
        try {
            size_t p = 0;
            long v = std::stol(s, &p);
            if (p == s.size() && v >= lo && v <= hi) return (int)v;
        } catch (...) {}
        std::cout << "  Invalid input. Please enter a number from " << lo << " to " << hi << ".\n";
    }
}
inline bool confirm(const std::string& prompt) {
    while (true) {
        std::string s = toLower(trim(readLine(prompt + " (y/n): ")));
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        std::cout << "  Please type y or n.\n";
    }
}

}  // namespace Utils


// ############ include/UI.h ############
// UI.h - console look & feel: clear screen, colours, banners, tables, pauses.
// Presentation only. No library logic lives here.

#ifdef _WIN32
#include <windows.h>
#endif

namespace UI {

const int WIDTH = 78;  // width of every banner / box

// ---------------- colours ----------------
enum Color { DEFAULT = 7, BLUE = 9, GREEN = 10, CYAN = 11, RED = 12, YELLOW = 14, WHITE = 15 };

inline void setColor(Color c) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)c);
#else
    const char* code = "0";
    switch (c) {
        case BLUE: code = "94"; break;
        case GREEN: code = "92"; break;
        case CYAN: code = "96"; break;
        case RED: code = "91"; break;
        case YELLOW: code = "93"; break;
        case WHITE: code = "97"; break;
        default: code = "0"; break;
    }
    std::cout << "\033[" << code << "m";
#endif
}
inline void resetColor() { setColor(DEFAULT); }

// ---------------- screen ----------------
inline void clearScreen() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(h, &info)) { std::system("cls"); return; }
    DWORD cells = (DWORD)info.dwSize.X * info.dwSize.Y, written;
    COORD home = {0, 0};
    FillConsoleOutputCharacterA(h, ' ', cells, home, &written);
    FillConsoleOutputAttribute(h, info.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(h, home);
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif
}

inline void pause(const std::string& text = "Press Enter to continue...") {
    setColor(YELLOW);
    std::cout << "\n  " << text << std::flush;
    resetColor();
    std::string dummy;
    std::getline(std::cin, dummy);
}

// ---------------- small helpers ----------------
inline std::string line(char c, int n = WIDTH) { return std::string(n, c); }

inline std::string center(const std::string& s, int w = WIDTH) {
    int n = (int)s.size();
    if (n >= w) return s;
    int left = (w - n) / 2;
    return std::string(left, ' ') + s + std::string(w - n - left, ' ');
}

// ---------------- banners and headings ----------------
// Big banner:   +----------+
//               |  TITLE   |
//               +----------+
inline void banner(const std::string& title, const std::string& subtitle = "") {
    setColor(CYAN);
    std::cout << "\n  +" << line('=', WIDTH - 2) << "+\n";
    std::cout << "  |";
    setColor(WHITE);
    std::cout << center(title, WIDTH - 2);
    setColor(CYAN);
    std::cout << "|\n";
    if (!subtitle.empty()) {
        std::cout << "  |";
        setColor(DEFAULT);
        std::cout << center(subtitle, WIDTH - 2);
        setColor(CYAN);
        std::cout << "|\n";
    }
    std::cout << "  +" << line('=', WIDTH - 2) << "+\n";
    resetColor();
}

// Screen heading: starts a fresh screen with a title bar (and who is logged in)
inline void screen(const std::string& title, const std::string& userInfo = "") {
    clearScreen();
    banner("LIBRARY MANAGEMENT SYSTEM");
    setColor(YELLOW);
    std::cout << "\n  >> " << title;
    resetColor();
    if (!userInfo.empty()) {
        setColor(DEFAULT);
        std::cout << "   [" << userInfo << "]";
    }
    std::cout << "\n  " << line('-', WIDTH - 2) << "\n";
    resetColor();
}

inline void section(const std::string& text) {
    setColor(CYAN);
    std::cout << "\n  " << text << "\n  " << line('-', (int)text.size()) << "\n";
    resetColor();
}

// ---------------- menu ----------------
inline void menuItem(const std::string& key, const std::string& text) {
    std::cout << "     ";
    setColor(YELLOW);
    std::cout << "[" << key << "]";
    resetColor();
    std::cout << " " << text << "\n";
}
inline void menuHeading(const std::string& text) {
    setColor(CYAN);
    std::cout << "\n   " << text << "\n";
    resetColor();
}
inline void menuItemPair(const std::string& k1, const std::string& t1, const std::string& k2,
                         const std::string& t2) {
    std::cout << "     ";
    setColor(YELLOW);
    std::cout << "[" << (k1.size() < 2 ? " " : "") << k1 << "]";
    resetColor();
    if (k2.empty()) { std::cout << " " << t1 << "\n"; return; }
    std::string a = " " + t1;
    a.resize(34, ' ');
    std::cout << a;
    setColor(YELLOW);
    std::cout << "[" << (k2.size() < 2 ? " " : "") << k2 << "]";
    resetColor();
    std::cout << " " << t2 << "\n";
}

// ---------------- messages ----------------
inline void success(const std::string& msg) {
    std::cout << "\n  ";
    setColor(GREEN);
    std::cout << "[ SUCCESS ] ";
    resetColor();
    std::cout << msg << "\n";
}
inline void error(const std::string& msg) {
    std::cout << "\n  ";
    setColor(RED);
    std::cout << "[ ERROR ]   ";
    resetColor();
    std::cout << msg << "\n";
}
inline void info(const std::string& msg) {
    std::cout << "\n  ";
    setColor(CYAN);
    std::cout << "[ INFO ]    ";
    resetColor();
    std::cout << msg << "\n";
}
inline void warning(const std::string& msg) {
    std::cout << "\n  ";
    setColor(YELLOW);
    std::cout << "[ NOTICE ]  ";
    resetColor();
    std::cout << msg << "\n";
}
inline void result(const Result& r) { r.ok ? success(r.msg) : error(r.msg); }

// ---------------- tables ----------------
// Usage: Table t({{"ID",6},{"Title",30}}); t.header(); t.row({"101","Clean Code"}); t.footer();
struct Column {
    std::string name;
    int width;  // inner width of the cell
};

class Table {
    std::vector<Column> cols;

    std::string border() const {
        std::string s = "  +";
        for (const auto& c : cols) s += std::string(c.width + 2, '-') + "+";
        return s;
    }

public:
    explicit Table(const std::vector<Column>& c) : cols(c) {}

    int totalWidth() const {
        int w = 3;
        for (const auto& c : cols) w += c.width + 3;
        return w;
    }

    void header() const {
        setColor(CYAN);
        std::cout << "\n" << border() << "\n  |";
        for (const auto& c : cols) {
            setColor(WHITE);
            std::string n = Utils::fit(c.name, (size_t)c.width);
            std::cout << " " << n << std::string(c.width - n.size(), ' ') << " ";
            setColor(CYAN);
            std::cout << "|";
        }
        std::cout << "\n" << border() << "\n";
        resetColor();
    }

    // colorIndex = which cell to colour (-1 = none)
    void row(const std::vector<std::string>& cells, int colorIndex = -1, Color color = DEFAULT) const {
        setColor(CYAN);
        std::cout << "  |";
        for (size_t i = 0; i < cols.size(); ++i) {
            std::string t = i < cells.size() ? Utils::fit(cells[i], (size_t)cols[i].width) : "";
            resetColor();
            std::cout << " ";
            if ((int)i == colorIndex) setColor(color);
            std::cout << t << std::string(cols[i].width - t.size(), ' ');
            resetColor();
            std::cout << " ";
            setColor(CYAN);
            std::cout << "|";
        }
        std::cout << "\n";
        resetColor();
    }

    void footer(const std::string& text = "") const {
        setColor(CYAN);
        std::cout << border() << "\n";
        resetColor();
        if (!text.empty()) std::cout << "  " << text << "\n";
    }
};

// Label / value pair used for "details" blocks
inline void field(const std::string& label, const std::string& value) {
    std::string l = label;
    l.resize(16, ' ');
    std::cout << "  ";
    setColor(CYAN);
    std::cout << l;
    resetColor();
    std::cout << ": " << value << "\n";
}

}  // namespace UI


// ############ include/Book.h ############
// Book.h - one book in the library

class Book {
private:
    int bookId;
    std::string title;
    std::string author;
    std::string category;
    bool available;

public:
    Book();
    Book(int id, const std::string& title, const std::string& author,
         const std::string& category, bool available = true);

    int getId() const { return bookId; }
    const std::string& getTitle() const { return title; }
    const std::string& getAuthor() const { return author; }
    const std::string& getCategory() const { return category; }
    void setTitle(const std::string& t) { title = t; }
    void setAuthor(const std::string& a) { author = a; }
    void setCategory(const std::string& c) { category = c; }

    bool isAvailable() const { return available; }
    void markIssued() { available = false; }
    void markReturned() { available = true; }

    void display() const;
    static void printHeader();

    // books.txt line: bookId|title|author|category|availability
    std::string toRecord() const;
    static bool fromRecord(const std::string& line, Book& out);
};


// ############ include/Student.h ############
// Student.h - a library member who borrows books

class Student {
private:
    int studentId;
    std::string name;
    std::string contact;
    std::vector<int> issuedBooks;  // IDs of books currently held

public:
    Student();
    Student(int id, const std::string& name, const std::string& contact);

    int getId() const { return studentId; }
    const std::string& getName() const { return name; }
    const std::string& getContact() const { return contact; }
    void setName(const std::string& n) { name = n; }
    void setContact(const std::string& c) { contact = c; }

    const std::vector<int>& getIssuedBooks() const { return issuedBooks; }
    int issuedCount() const { return (int)issuedBooks.size(); }
    bool hasBook(int bookId) const;
    void addIssuedBook(int bookId);
    void removeIssuedBook(int bookId);
    void clearIssued() { issuedBooks.clear(); }
    void viewIssuedBooks() const;

    void display() const;
    static void printHeader();

    // students.txt line: studentId|name|contact|issuedBooks  (e.g. 201|Asha|98765|101,102 or -)
    std::string toRecord() const;
    static bool fromRecord(const std::string& line, Student& out);
};


// ############ include/Transaction.h ############
// Transaction.h - one borrowing / returning record (also does the fine maths)

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


// ############ include/FileManager.h ############
// FileManager.h - saves data to text files and loads it again

class FileManager {
private:
    std::string dataFolder;
    mutable int skippedLines;  // damaged lines ignored while loading

    bool writeLines(const std::string& fileName, const std::vector<std::string>& lines) const;
    std::vector<std::string> readLines(const std::string& fileName) const;

public:
    explicit FileManager(const std::string& folder = "data");

    bool ensureFolder() const;
    int getSkippedLines() const { return skippedLines; }
    void resetSkipped() const { skippedLines = 0; }

    bool saveBooks(const std::vector<Book>& books) const;
    std::vector<Book> loadBooks() const;
    bool saveStudents(const std::vector<Student>& students) const;
    std::vector<Student> loadStudents() const;
    bool saveTransactions(const std::vector<Transaction>& transactions) const;
    std::vector<Transaction> loadTransactions() const;
};


// ############ include/Librarian.h ############
// Librarian.h - the single administrator of the library

class Library;  // forward declaration (Library owns the Librarian)

class Librarian {
private:
    std::string username;
    std::string passwordHash;  // never the plain password

public:
    Librarian();  // default account: admin / admin123
    Librarian(const std::string& user, const std::string& passwordPlain);

    bool login(const std::string& user, const std::string& password) const;

    // management operations (they pass the work on to Library)
    int addBook(Library& lib, const std::string& title, const std::string& author,
                const std::string& category);
    int registerStudent(Library& lib, const std::string& name, const std::string& contact);
    Result issueBook(Library& lib, int studentId, int bookId, const std::string& date = "");
    Result returnBook(Library& lib, int studentId, int bookId, const std::string& date, int& fine);
    void viewTransactions(const Library& lib) const;
};


// ############ include/Library.h ############
// Library.h - the main class: connects books, students, transactions and files

enum class SearchField { All, Title, Author, Category, Id };
enum class AvailFilter { Any, AvailableOnly, IssuedOnly };

class Library {
private:
    std::vector<Book> books;
    std::vector<Student> students;
    std::vector<Transaction> transactions;
    Librarian librarian;
    FileManager fileManager;
    bool persistent;  // false = keep everything in memory only (used by the self-test)

    int nextBookId() const;
    int nextStudentId() const;
    int nextTransactionId() const;
    Book* findBookPtr(int id);
    Student* findStudentPtr(int id);
    void persist();
    void reconcile();  // make books/students agree with open transactions
    void printTransactions(const std::vector<const Transaction*>& list, long asOf) const;

public:
    explicit Library(const std::string& dataFolder = "data", bool persistent = true);

    // ---- files ----
    bool load();
    bool save();
    bool isEmpty() const { return books.empty() && students.empty(); }
    void loadSampleData();

    Librarian& getLibrarian() { return librarian; }
    const Book* findBook(int id) const;
    const Student* findStudent(int id) const;
    size_t bookCount() const { return books.size(); }
    size_t studentCount() const { return students.size(); }
    size_t transactionCount() const { return transactions.size(); }

    // ---- books ----
    int addBook(const std::string& title, const std::string& author, const std::string& category);
    Result updateBook(int id, const std::string& title, const std::string& author,
                      const std::string& category);  // empty text = keep old value
    Result removeBook(int id);

    // ---- students ----
    int registerStudent(const std::string& name, const std::string& contact);
    Result updateStudent(int id, const std::string& name, const std::string& contact);
    Result deleteStudent(int id);

    // ---- issue / return with fine calculation (Milestone 3) ----
    // date = "" means today; otherwise YYYY-MM-DD
    Result issueBook(int studentId, int bookId, const std::string& date = "");
    Result returnBook(int studentId, int bookId, const std::string& date, int& fineOut);

    // ---- search and filter (Milestone 3) ----
    std::vector<const Book*> searchBooks(const std::string& keyword,
                                         SearchField field = SearchField::All,
                                         const std::string& category = "",
                                         AvailFilter avail = AvailFilter::Any) const;
    std::vector<std::string> categories() const;
    std::vector<const Transaction*> overdueTransactions(long asOf) const;

    // ---- screens ----
    void printBooks(const std::vector<const Book*>& list) const;
    void showAllBooks() const;
    void showStudents() const;
    void showTransactions() const;
    void showIssuedBooks() const;
    void showOverdueBooks(long asOf) const;
    void showStudentIssued(int studentId, long asOf) const;
};


// ############ include/Menu.h ############
// Menu.h - SCREEN LAYER: text menus in the terminal

class Menu {
private:
    Library& lib;

    void librarianMenu();
    void studentMenu(int studentId);
    void reportsMenu();
    void searchAndFilterScreen();

    // librarian actions
    void addBookScreen();
    void updateBookScreen();
    void removeBookScreen();
    void registerStudentScreen();
    void updateStudentScreen();
    void deleteStudentScreen();
    void issueBookScreen();
    void returnBookScreen();

public:
    explicit Menu(Library& library) : lib(library) {}
    void run();
};


// ############ include/Demo.h ############
// Demo.h - prototype demonstration / self-test of the core logic
int runDemo();  // returns 0 when every check passes


// ############ src/Book.cpp ############

Book::Book() : bookId(0), available(true) {}

Book::Book(int id, const std::string& t, const std::string& a, const std::string& c, bool avail)
    : bookId(id), title(t), author(a), category(c), available(avail) {}

static UI::Table bookTable() {
    return UI::Table({{"ID", 5}, {"Title", 27}, {"Author", 22}, {"Category", 16}, {"Status", 9}});
}

void Book::printHeader() { bookTable().header(); }

void Book::display() const {
    bookTable().row({std::to_string(bookId), title, author, category,
                     available ? "Available" : "Issued"},
                    4, available ? UI::GREEN : UI::YELLOW);
}

std::string Book::toRecord() const {
    return std::to_string(bookId) + "|" + title + "|" + author + "|" + category + "|" +
           (available ? "Available" : "Issued");
}

bool Book::fromRecord(const std::string& line, Book& out) {
    std::vector<std::string> p = Utils::split(line, '|');
    int id;
    if (p.size() != 5 || !Utils::toInt(p[0], id) || p[1].empty()) return false;
    std::string st = Utils::toLower(Utils::trim(p[4]));
    if (st != "available" && st != "issued") return false;
    out = Book(id, p[1], p[2], p[3], st == "available");
    return true;
}


// ############ src/Student.cpp ############

Student::Student() : studentId(0) {}

Student::Student(int id, const std::string& n, const std::string& c)
    : studentId(id), name(n), contact(c) {}

bool Student::hasBook(int bookId) const {
    return std::find(issuedBooks.begin(), issuedBooks.end(), bookId) != issuedBooks.end();
}

void Student::addIssuedBook(int bookId) {
    if (!hasBook(bookId)) issuedBooks.push_back(bookId);
}

void Student::removeIssuedBook(int bookId) {
    issuedBooks.erase(std::remove(issuedBooks.begin(), issuedBooks.end(), bookId),
                      issuedBooks.end());
}

void Student::viewIssuedBooks() const {
    if (issuedBooks.empty()) { UI::info("No books issued."); return; }
    std::cout << "  Issued book IDs:";
    for (int b : issuedBooks) std::cout << " " << b;
    std::cout << "\n";
}

static UI::Table studentTable() {
    return UI::Table({{"ID", 5}, {"Name", 26}, {"Contact", 15}, {"Books held", 10}});
}

void Student::printHeader() { studentTable().header(); }

void Student::display() const {
    studentTable().row({std::to_string(studentId), name, contact,
                        std::to_string(issuedBooks.size()) + "/" + std::to_string(Config::MAX_BOOKS)});
}

std::string Student::toRecord() const {
    std::string books;
    for (size_t i = 0; i < issuedBooks.size(); ++i) {
        if (i) books += ",";
        books += std::to_string(issuedBooks[i]);
    }
    if (books.empty()) books = "-";
    return std::to_string(studentId) + "|" + name + "|" + contact + "|" + books;
}

bool Student::fromRecord(const std::string& line, Student& out) {
    std::vector<std::string> p = Utils::split(line, '|');
    int id;
    if (p.size() != 4 || !Utils::toInt(p[0], id) || p[1].empty()) return false;
    Student s(id, p[1], p[2]);
    std::string list = Utils::trim(p[3]);
    if (!list.empty() && list != "-" && Utils::toLower(list) != "none") {
        for (const std::string& part : Utils::split(list, ',')) {
            int b;
            if (!Utils::toInt(part, b)) return false;
            s.addIssuedBook(b);
        }
    }
    out = s;
    return true;
}


// ############ src/Transaction.cpp ############

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


// ############ src/FileManager.cpp ############

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& folder) : dataFolder(folder), skippedLines(0) {}

bool FileManager::ensureFolder() const {
    std::error_code ec;
    fs::create_directories(dataFolder, ec);
    return !ec;
}

// Write to a temporary file first, then swap it in. If the program is closed in the
// middle of saving, the old file is still complete (requirement: "safe data").
bool FileManager::writeLines(const std::string& fileName,
                             const std::vector<std::string>& lines) const {
    if (!ensureFolder()) return false;
    const std::string path = dataFolder + "/" + fileName;
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) return false;
        for (const auto& l : lines) out << l << '\n';
        out.flush();
        if (!out) return false;
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) {  // some systems refuse to overwrite: remove old file and retry
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
    }
    return !ec;
}

std::vector<std::string> FileManager::readLines(const std::string& fileName) const {
    std::vector<std::string> lines;
    std::ifstream in(dataFolder + "/" + fileName);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

bool FileManager::saveBooks(const std::vector<Book>& v) const {
    std::vector<std::string> l;
    for (const auto& x : v) l.push_back(x.toRecord());
    return writeLines("books.txt", l);
}
std::vector<Book> FileManager::loadBooks() const {
    std::vector<Book> v;
    for (const auto& line : readLines("books.txt")) {
        Book b;
        if (Book::fromRecord(line, b)) v.push_back(b); else ++skippedLines;
    }
    return v;
}

bool FileManager::saveStudents(const std::vector<Student>& v) const {
    std::vector<std::string> l;
    for (const auto& x : v) l.push_back(x.toRecord());
    return writeLines("students.txt", l);
}
std::vector<Student> FileManager::loadStudents() const {
    std::vector<Student> v;
    for (const auto& line : readLines("students.txt")) {
        Student s;
        if (Student::fromRecord(line, s)) v.push_back(s); else ++skippedLines;
    }
    return v;
}

bool FileManager::saveTransactions(const std::vector<Transaction>& v) const {
    std::vector<std::string> l;
    for (const auto& x : v) l.push_back(x.toRecord());
    return writeLines("transactions.txt", l);
}
std::vector<Transaction> FileManager::loadTransactions() const {
    std::vector<Transaction> v;
    for (const auto& line : readLines("transactions.txt")) {
        Transaction t;
        if (Transaction::fromRecord(line, t)) v.push_back(t); else ++skippedLines;
    }
    return v;
}


// ############ src/Librarian.cpp ############

Librarian::Librarian() : username("admin"), passwordHash(Utils::hashPassword("admin123")) {}

Librarian::Librarian(const std::string& user, const std::string& passwordPlain)
    : username(user), passwordHash(Utils::hashPassword(passwordPlain)) {}

bool Librarian::login(const std::string& user, const std::string& password) const {
    return user == username && Utils::hashPassword(password) == passwordHash;
}

int Librarian::addBook(Library& lib, const std::string& t, const std::string& a,
                       const std::string& c) {
    return lib.addBook(t, a, c);
}
int Librarian::registerStudent(Library& lib, const std::string& n, const std::string& c) {
    return lib.registerStudent(n, c);
}
Result Librarian::issueBook(Library& lib, int sid, int bid, const std::string& date) {
    return lib.issueBook(sid, bid, date);
}
Result Librarian::returnBook(Library& lib, int sid, int bid, const std::string& date,
                             int& fine) {
    return lib.returnBook(sid, bid, date, fine);
}
void Librarian::viewTransactions(const Library& lib) const { lib.showTransactions(); }


// ############ src/Library.cpp ############

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


// ############ src/Menu.cpp ############

using namespace std;

// ---------------------------------------------------------------- small helpers
static void rules() {
    UI::setColor(UI::DEFAULT);
    cout << "\n  Library rules : keep a book " << Config::LOAN_DAYS << " days  |  max "
         << Config::MAX_BOOKS << " books  |  late fine Rs. " << Config::FINE_PER_DAY << "/day\n";
}

static int choose(int lo, int hi) {
    cout << "\n";
    UI::setColor(UI::YELLOW);
    cout << "  Enter your choice";
    UI::resetColor();
    return Utils::readInt(" (" + to_string(lo) + "-" + to_string(hi) + "): ", lo, hi);
}

static void say(const Result& r) { UI::result(r); }

// ---------------------------------------------------------------- main menu
void Menu::run() {
    while (true) {
        UI::clearScreen();
        UI::banner("LIBRARY MANAGEMENT SYSTEM", "Books  |  Students  |  Issue & Return  |  Fines");
        rules();
        UI::menuHeading("MAIN MENU");
        UI::menuItem("1", "Librarian login");
        UI::menuItem("2", "Student login");
        UI::menuItem("3", "Run prototype self-test (demo)");
        UI::menuItem("0", "Exit");
        int c = choose(0, 3);

        if (c == 0) {
            lib.save();
            UI::screen("Goodbye");
            UI::success("All data saved. Thank you for using the Library Management System!");
            cout << "\n";
            return;
        }
        if (c == 1) {
            UI::screen("Librarian Login");
            cout << "\n";
            bool ok = false;
            for (int attempt = 1; attempt <= 3 && !ok; ++attempt) {
                string u = Utils::trim(Utils::readLine("  User name : "));
                string p = Utils::readLine("  Password  : ");
                ok = lib.getLibrarian().login(u, p);
                if (!ok) {
                    UI::error("Wrong user name or password (" + to_string(attempt) + "/3).");
                    cout << "\n";
                }
            }
            if (ok) {
                librarianMenu();
            } else {
                UI::error("Too many wrong attempts. Returning to the main menu.");
                UI::pause();
            }
        } else if (c == 2) {
            UI::screen("Student Login");
            cout << "\n";
            int id = Utils::readInt("  Enter your student ID: ", 0, 2000000000);
            const Student* s = lib.findStudent(id);
            if (!s) {
                UI::error("Student ID " + to_string(id) + " is not registered. Ask the librarian.");
                UI::pause();
            } else {
                studentMenu(id);
            }
        } else {
            UI::screen("Prototype Self-Test");
            runDemo();
            UI::pause();
        }
    }
}

// ---------------------------------------------------------------- librarian
void Menu::librarianMenu() {
    bool firstTime = true;
    while (true) {
        UI::screen("Librarian Dashboard", "Logged in as Librarian");
        if (firstTime) {
            UI::success("Login successful. Welcome, Librarian!");
            firstTime = false;
        }
        UI::menuHeading("BOOKS");
        UI::menuItemPair("1", "Add book", "4", "View all books");
        UI::menuItemPair("2", "Update book", "5", "Search & filter books");
        UI::menuItemPair("3", "Remove book", "", "");
        UI::menuHeading("STUDENTS");
        UI::menuItemPair("6", "Register student", "9", "View all students");
        UI::menuItemPair("7", "Update student", "", "");
        UI::menuItemPair("8", "Delete student", "", "");
        UI::menuHeading("ISSUE / RETURN");
        UI::menuItemPair("10", "Issue book", "12", "View all transactions");
        UI::menuItemPair("11", "Return book (with fine)", "13", "Reports (issued / overdue)");
        cout << "\n";
        UI::menuItemPair("0", "Logout", "", "");
        int c = choose(0, 13);
        switch (c) {
            case 0:
                UI::screen("Logged Out");
                UI::info("You have been logged out.");
                UI::pause();
                return;
            case 1: addBookScreen(); break;
            case 2: updateBookScreen(); break;
            case 3: removeBookScreen(); break;
            case 4:
                UI::screen("All Books");
                lib.showAllBooks();
                UI::pause();
                break;
            case 5: searchAndFilterScreen(); break;
            case 6: registerStudentScreen(); break;
            case 7: updateStudentScreen(); break;
            case 8: deleteStudentScreen(); break;
            case 9:
                UI::screen("All Students");
                lib.showStudents();
                UI::pause();
                break;
            case 10: issueBookScreen(); break;
            case 11: returnBookScreen(); break;
            case 12:
                UI::screen("All Transactions");
                lib.getLibrarian().viewTransactions(lib);
                UI::pause();
                break;
            case 13: reportsMenu(); break;
        }
    }
}

void Menu::reportsMenu() {
    while (true) {
        UI::screen("Reports", "Librarian");
        UI::menuHeading("CHOOSE A REPORT");
        UI::menuItem("1", "Books currently issued");
        UI::menuItem("2", "Overdue (late) books");
        UI::menuItem("3", "All books");
        UI::menuItem("0", "Back to dashboard");
        int c = choose(0, 3);
        if (c == 0) return;
        if (c == 1) {
            UI::screen("Report: Books Currently Issued", "Librarian");
            lib.showIssuedBooks();
        } else if (c == 2) {
            UI::screen("Report: Overdue Books", "Librarian");
            UI::info("Overdue as of today (" + Utils::todayString() + "):");
            lib.showOverdueBooks(Utils::todayDays());
        } else {
            UI::screen("Report: All Books", "Librarian");
            lib.showAllBooks();
        }
        UI::pause();
    }
}

// ---------------------------------------------------------------- book screens
void Menu::addBookScreen() {
    UI::screen("Add New Book", "Librarian");
    cout << "\n";
    string t = Utils::readNonEmpty("  Title    : ");
    string a = Utils::readNonEmpty("  Author   : ");
    string c = Utils::readNonEmpty("  Category : ");
    int id = lib.getLibrarian().addBook(lib, t, a, c);
    UI::success("Book added with ID " + to_string(id) + ".");
    UI::pause();
}

void Menu::updateBookScreen() {
    UI::screen("Update Book", "Librarian");
    cout << "\n";
    int id = Utils::readInt("  Book ID to update: ", 0, 2000000000);
    const Book* b = lib.findBook(id);
    if (!b) {
        UI::error("No book with ID " + to_string(id) + ".");
        UI::pause();
        return;
    }
    UI::section("Current details");
    UI::field("Title", b->getTitle());
    UI::field("Author", b->getAuthor());
    UI::field("Category", b->getCategory());
    UI::warning("Press Enter on any question to keep the current value.");
    cout << "\n";
    string t = Utils::readLine("  New title    : ");
    string a = Utils::readLine("  New author   : ");
    string c = Utils::readLine("  New category : ");
    say(lib.updateBook(id, t, a, c));
    UI::pause();
}

void Menu::removeBookScreen() {
    UI::screen("Remove Book", "Librarian");
    cout << "\n";
    int id = Utils::readInt("  Book ID to remove: ", 0, 2000000000);
    const Book* b = lib.findBook(id);
    if (!b) {
        UI::error("No book with ID " + to_string(id) + ".");
        UI::pause();
        return;
    }
    cout << "\n";
    if (!Utils::confirm("  Remove \"" + b->getTitle() + "\"?")) {
        UI::info("Cancelled. Nothing was removed.");
        UI::pause();
        return;
    }
    say(lib.removeBook(id));
    UI::pause();
}

// ---------------------------------------------------------------- student screens
static string readContact() {
    while (true) {
        string c = Utils::sanitize(Utils::readLine("  Contact (phone, digits only): "));
        bool digits = !c.empty();
        for (char ch : c) if (!isdigit((unsigned char)ch)) digits = false;
        if (digits && c.size() >= 7 && c.size() <= 15) return c;
        UI::error("Enter 7 to 15 digits.");
    }
}

void Menu::registerStudentScreen() {
    UI::screen("Register New Student", "Librarian");
    cout << "\n";
    string n = Utils::readNonEmpty("  Student name : ");
    string c = readContact();
    int id = lib.getLibrarian().registerStudent(lib, n, c);
    UI::success("Student registered. Student ID = " + to_string(id) + ".");
    UI::pause();
}

void Menu::updateStudentScreen() {
    UI::screen("Update Student", "Librarian");
    cout << "\n";
    int id = Utils::readInt("  Student ID to update: ", 0, 2000000000);
    const Student* s = lib.findStudent(id);
    if (!s) {
        UI::error("No student with ID " + to_string(id) + ".");
        UI::pause();
        return;
    }
    UI::section("Current details");
    UI::field("Name", s->getName());
    UI::field("Contact", s->getContact());
    UI::warning("Press Enter on any question to keep the current value.");
    cout << "\n";
    string n = Utils::readLine("  New name    : ");
    string c = Utils::readLine("  New contact : ");
    say(lib.updateStudent(id, n, c));
    UI::pause();
}

void Menu::deleteStudentScreen() {
    UI::screen("Delete Student", "Librarian");
    cout << "\n";
    int id = Utils::readInt("  Student ID to delete: ", 0, 2000000000);
    const Student* s = lib.findStudent(id);
    if (!s) {
        UI::error("No student with ID " + to_string(id) + ".");
        UI::pause();
        return;
    }
    cout << "\n";
    if (!Utils::confirm("  Delete " + s->getName() + "?")) {
        UI::info("Cancelled. Nothing was deleted.");
        UI::pause();
        return;
    }
    say(lib.deleteStudent(id));
    UI::pause();
}

// ---------------------------------------------------------------- issue / return
void Menu::issueBookScreen() {
    UI::screen("Issue Book", "Librarian");
    UI::info("Loan period: " + to_string(Config::LOAN_DAYS) + " days. Max " +
             to_string(Config::MAX_BOOKS) + " books per student.");
    cout << "\n";
    int sid = Utils::readInt("  Student ID : ", 0, 2000000000);
    int bid = Utils::readInt("  Book ID    : ", 0, 2000000000);
    string d = Utils::readLine("  Issue date YYYY-MM-DD (Enter = today): ");
    say(lib.getLibrarian().issueBook(lib, sid, bid, d));
    UI::pause();
}

void Menu::returnBookScreen() {
    UI::screen("Return Book", "Librarian");
    UI::info("Late fine: Rs. " + to_string(Config::FINE_PER_DAY) + " per day after " +
             to_string(Config::LOAN_DAYS) + " days.");
    cout << "\n";
    int sid = Utils::readInt("  Student ID : ", 0, 2000000000);
    int bid = Utils::readInt("  Book ID    : ", 0, 2000000000);
    string d = Utils::readLine("  Return date YYYY-MM-DD (Enter = today): ");
    int fine = 0;
    Result r = lib.getLibrarian().returnBook(lib, sid, bid, d, fine);
    say(r);
    if (r.ok && fine > 0) UI::warning("Collect Rs. " + to_string(fine) + " from the student.");
    UI::pause();
}

// ---------------------------------------------------------------- search & filter
void Menu::searchAndFilterScreen() {
    UI::screen("Search & Filter Books");
    UI::info("Leave a question empty to skip it.");
    cout << "\n";
    string kw = Utils::trim(Utils::readLine("  Search word: "));
    SearchField field = SearchField::All;
    if (!kw.empty()) {
        UI::section("Search in");
        UI::menuItemPair("1", "Everything", "2", "Title");
        UI::menuItemPair("3", "Author", "4", "Category");
        UI::menuItem("5", "Book ID");
        switch (choose(1, 5)) {
            case 2: field = SearchField::Title; break;
            case 3: field = SearchField::Author; break;
            case 4: field = SearchField::Category; break;
            case 5: field = SearchField::Id; break;
            default: break;
        }
    }
    vector<string> cats = lib.categories();
    string cat;
    if (!cats.empty()) {
        UI::section("Categories");
        cout << " ";
        for (const auto& c : cats) {
            cout << " ";
            UI::setColor(UI::YELLOW);
            cout << "[" << c << "]";
            UI::resetColor();
        }
        cout << "\n\n";
        cat = Utils::trim(Utils::readLine("  Filter by category (Enter = any): "));
    }
    UI::section("Show");
    UI::menuItemPair("1", "All books", "2", "Available only");
    UI::menuItem("3", "Issued only");
    AvailFilter af = AvailFilter::Any;
    int a = choose(1, 3);
    if (a == 2) af = AvailFilter::AvailableOnly;
    if (a == 3) af = AvailFilter::IssuedOnly;

    UI::screen("Search Results");
    lib.printBooks(lib.searchBooks(kw, field, cat, af));
    UI::pause();
}

// ---------------------------------------------------------------- student
void Menu::studentMenu(int studentId) {
    bool firstTime = true;
    while (true) {
        const Student* me = lib.findStudent(studentId);
        if (!me) return;
        string who = me->getName() + " (ID " + to_string(studentId) + ")";
        UI::screen("Student Dashboard", who);
        if (firstTime) {
            UI::success("Welcome, " + me->getName() + "!");
            firstTime = false;
        }
        UI::menuHeading("WHAT WOULD YOU LIKE TO DO?");
        UI::menuItem("1", "Search & filter books");
        UI::menuItem("2", "See available books");
        UI::menuItem("3", "Borrow a book");
        UI::menuItem("4", "Return a book");
        UI::menuItem("5", "My issued books (due dates & fines)");
        UI::menuItem("0", "Logout");
        int c = choose(0, 5);
        if (c == 0) {
            UI::screen("Logged Out");
            UI::info("You have been logged out.");
            UI::pause();
            return;
        }
        if (c == 1) {
            searchAndFilterScreen();
        } else if (c == 2) {
            UI::screen("Available Books", who);
            lib.printBooks(lib.searchBooks("", SearchField::All, "", AvailFilter::AvailableOnly));
            UI::pause();
        } else if (c == 3) {
            UI::screen("Borrow a Book", who);
            cout << "\n";
            int bid = Utils::readInt("  Book ID to borrow: ", 0, 2000000000);
            say(lib.issueBook(studentId, bid, ""));
            UI::pause();
        } else if (c == 4) {
            UI::screen("Return a Book", who);
            cout << "\n";
            int bid = Utils::readInt("  Book ID to return: ", 0, 2000000000);
            int fine = 0;
            Result r = lib.returnBook(studentId, bid, "", fine);
            say(r);
            if (r.ok && fine > 0)
                UI::warning("Please pay Rs. " + to_string(fine) + " at the library desk.");
            UI::pause();
        } else if (c == 5) {
            UI::screen("My Issued Books", who);
            lib.showStudentIssued(studentId, Utils::todayDays());
            UI::pause();
        }
    }
}


// ############ src/Demo.cpp ############
// Prototype demonstration: runs the core logic on a throw-away in-memory library
// with FIXED dates, so the results are the same every time it is run.

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


// ############ src/main.cpp ############
// Library Management System - C++ console application
// Layers: Menu (screen)  ->  Library/Librarian/Student/Book/Transaction (logic)  ->  FileManager (files)

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--demo") return runDemo();

#ifdef _WIN32
    std::system("mode con: cols=120 lines=40");  // wide window so tables never wrap
    std::system("title Library Management System");
#endif

    Library library("data", true);
    library.load();

    if (library.isEmpty()) {
        UI::screen("First Run Setup");
        UI::info("No library data found (first run).");
        std::cout << "\n";
        if (Utils::confirm("  Load sample books and students so you can try the system?"))
            library.loadSampleData();
    }

    UI::screen("Welcome");
    UI::info("Default librarian login ->  user: admin   password: admin123");
    UI::pause();

    Menu menu(library);
    menu.run();
    return 0;
}
