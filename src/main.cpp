// Library Management System - C++ console application
// Layers: Menu (screen)  ->  Library/Librarian/Student/Book/Transaction (logic)  ->  FileManager (files)
#include <iostream>
#include <string>
#include "Demo.h"
#include "Library.h"
#include "Menu.h"
#include "UI.h"
#include "Utils.h"

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--demo") return runDemo();

#ifdef _WIN32
    std::system("mode con: cols=120 lines=40");  // wide window so tables never wrap
    std::system("title Library Management System");
#endif

    Library library("data", true);
    library.load();

    if (library.isEmpty()) {
        UI::screen("First Run Setup");
        UI::info("No library data found (first run).");
        std::cout << "\n";
        if (Utils::confirm("  Load sample books and students so you can try the system?"))
            library.loadSampleData();
    }

    UI::screen("Welcome");
    UI::info("Default librarian login ->  user: admin   password: admin123");
    UI::pause();

    Menu menu(library);
    menu.run();
    return 0;
}
