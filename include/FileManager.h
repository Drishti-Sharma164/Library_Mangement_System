// FileManager.h - saves data to text files and loads it again
#pragma once
#include <string>
#include <vector>
#include "Book.h"
#include "Student.h"
#include "Transaction.h"

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
