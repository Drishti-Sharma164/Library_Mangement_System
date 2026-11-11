#include "Book.h"
#include <iomanip>
#include <iostream>
#include "UI.h"
#include "Utils.h"

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
