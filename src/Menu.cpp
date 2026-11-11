#include "Menu.h"
#include <iostream>
#include "Demo.h"
#include "UI.h"
#include "Utils.h"

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
