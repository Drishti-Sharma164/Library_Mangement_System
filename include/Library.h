// Library.h - the main class: connects books, students, transactions and files
#pragma once
#include <string>
#include <vector>
#include "Book.h"
#include "FileManager.h"
#include "Librarian.h"
#include "Student.h"
#include "Transaction.h"
#include "Utils.h"

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
