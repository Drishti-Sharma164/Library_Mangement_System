LIBRARY MANAGEMENT SYSTEM  (C++17 console application)
======================================================

HOW TO RUN (pick one)
---------------------
A) Easiest - single file (works on Windows, Linux, macOS):
       g++ -std=c++17 LibraryManagementSystem.cpp -o library
       ./library                (Windows: library.exe)

B) Full project (src/ + include/):
       make                     or:  g++ -std=c++17 -Iinclude src/*.cpp -o library
       ./library

C) Visual Studio / Code::Blocks / Dev-C++: add every file in src/ to a project, set the include
   folder to include/, and choose the C++17 standard.

CONSOLE UI
----------
Every screen clears the console, results are shown with [ SUCCESS ] / [ ERROR ] / [ INFO ] tags,
and "Press Enter to continue..." appears before the next menu. All look-and-feel code is in
include/UI.h (colours, banners, tables). On Windows the window is set to 120 columns wide.

FIRST RUN
---------
* The program creates the data/ folder by itself and offers sample books/students.
* Librarian login:  user = admin   password = admin123
* Students log in with their student ID (sample IDs: 201, 202, 203).
* Menu option 3 on the main screen (or  ./library --demo ) runs the automatic self-test.

LIBRARY RULES
-------------
Keep a book 14 days | at most 3 books per student | late fine Rs. 2 per day.
(Change them in include/Utils.h, namespace Config.)

DATA FILES (data/, one line = one record, parts separated by | )
  books.txt         bookId|title|author|category|availability
  students.txt      studentId|name|contact|issuedBooks   (e.g. 201|Asha|9876500011|101,102  or  -)
  transactions.txt  transactionId|studentId|bookId|issueDate|returnDate   (NULL = not returned)
Delete the data/ files to start fresh.
