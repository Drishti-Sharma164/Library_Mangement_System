#include "FileManager.h"
#include <filesystem>
#include <fstream>

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
