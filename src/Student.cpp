#include "Student.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include "UI.h"
#include "Utils.h"

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
