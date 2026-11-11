// Librarian.h - the single administrator of the library
#pragma once
#include <string>
#include "Utils.h"

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
