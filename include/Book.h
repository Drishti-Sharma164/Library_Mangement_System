// Book.h - one book in the library
#pragma once
#include <string>

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
