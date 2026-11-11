// Menu.h - SCREEN LAYER: text menus in the terminal
#pragma once
#include "Library.h"

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
