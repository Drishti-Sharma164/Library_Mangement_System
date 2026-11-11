// Student.h - a library member who borrows books
#pragma once
#include <string>
#include <vector>

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
