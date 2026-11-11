#include "Librarian.h"
#include "Library.h"

Librarian::Librarian() : username("admin"), passwordHash(Utils::hashPassword("admin123")) {}

Librarian::Librarian(const std::string& user, const std::string& passwordPlain)
    : username(user), passwordHash(Utils::hashPassword(passwordPlain)) {}

bool Librarian::login(const std::string& user, const std::string& password) const {
    return user == username && Utils::hashPassword(password) == passwordHash;
}

int Librarian::addBook(Library& lib, const std::string& t, const std::string& a,
                       const std::string& c) {
    return lib.addBook(t, a, c);
}
int Librarian::registerStudent(Library& lib, const std::string& n, const std::string& c) {
    return lib.registerStudent(n, c);
}
Result Librarian::issueBook(Library& lib, int sid, int bid, const std::string& date) {
    return lib.issueBook(sid, bid, date);
}
Result Librarian::returnBook(Library& lib, int sid, int bid, const std::string& date,
                             int& fine) {
    return lib.returnBook(sid, bid, date, fine);
}
void Librarian::viewTransactions(const Library& lib) const { lib.showTransactions(); }
