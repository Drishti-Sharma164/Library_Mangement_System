// Utils.h - shared constants, helpers for dates, text and console input
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ---------- Library rules (from the Milestone 1 design) ----------
namespace Config {
const int LOAN_DAYS = 14;     // a book may be kept for 14 days
const int MAX_BOOKS = 3;      // a student may hold at most 3 books
const int FINE_PER_DAY = 2;   // late fine: Rs. 2 per day
}

// Simple success/failure answer with a message for the screen
struct Result {
    bool ok;
    std::string msg;
};

namespace Utils {

// ---------- text helpers ----------
inline std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace((unsigned char)s[b])) ++b;
    while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}
inline std::string toLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
inline bool containsIgnoreCase(const std::string& hay, const std::string& needle) {
    return toLower(hay).find(toLower(needle)) != std::string::npos;
}
inline bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}
// '|' is the file separator, so it must never appear inside a field
inline std::string sanitize(std::string s) {
    s = trim(s);
    for (auto& c : s) if (c == '|' || c == '\n' || c == '\r') c = '/';
    return s;
}
inline bool toInt(const std::string& s, int& out) {
    std::string t = trim(s);
    if (t.empty()) return false;
    try {
        size_t p = 0;
        long v = std::stol(t, &p);
        if (p != t.size() || v < 0 || v > 2000000000L) return false;
        out = (int)v;
        return true;
    } catch (...) { return false; }
}
// shorten text so it fits a table column
inline std::string fit(const std::string& s, size_t w) {
    if (s.size() <= w) return s;
    if (w <= 2) return s.substr(0, w);
    return s.substr(0, w - 2) + "..";
}

// ---------- password hashing (FNV-1a, salted) ----------
// Learning-grade: passwords are never stored as plain text.
inline std::string hashPassword(const std::string& pw) {
    uint64_t h = 1469598103934665603ULL;
    const std::string salted = "LMS-salt-2026:" + pw;
    for (unsigned char c : salted) { h ^= c; h *= 1099511628211ULL; }
    std::ostringstream o;
    o << std::hex << std::setw(16) << std::setfill('0') << h;
    return o.str();
}

// ---------- dates (no time-zone tricks: pure calendar arithmetic) ----------
// days since 1970-01-01 for a calendar date (Howard Hinnant's algorithm)
inline long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
inline void civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long yy = yoe + era * 400;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    y = (int)(yy + (m <= 2));
}
// "YYYY-MM-DD" -> day number. Rejects things like 2026-02-30.
inline bool parseDate(const std::string& text, long& days) {
    std::string s = trim(text);
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < 10; ++i)
        if (i != 4 && i != 7 && !std::isdigit((unsigned char)s[i])) return false;
    int y = std::stoi(s.substr(0, 4));
    int m = std::stoi(s.substr(5, 2));
    int d = std::stoi(s.substr(8, 2));
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    long z = daysFromCivil(y, m, d);
    int y2, m2, d2;
    civilFromDays(z, y2, m2, d2);
    if (y2 != y || m2 != m || d2 != d) return false;
    days = z;
    return true;
}
inline std::string formatDate(long days) {
    int y, m, d;
    civilFromDays(days, y, m, d);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", y, m, d);
    return buf;
}
inline long todayDays() {
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    return daysFromCivil(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
}
inline std::string todayString() { return formatDate(todayDays()); }

// ---------- console input ----------
inline std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) {  // input closed (Ctrl+D / end of file)
        std::cout << "\nInput closed. Data is already saved. Goodbye!\n";
        std::exit(0);
    }
    return s;
}
inline std::string readNonEmpty(const std::string& prompt) {
    while (true) {
        std::string s = sanitize(readLine(prompt));
        if (!s.empty()) return s;
        std::cout << "  This field cannot be empty.\n";
    }
}
inline int readInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string s = trim(readLine(prompt));
        try {
            size_t p = 0;
            long v = std::stol(s, &p);
            if (p == s.size() && v >= lo && v <= hi) return (int)v;
        } catch (...) {}
        std::cout << "  Invalid input. Please enter a number from " << lo << " to " << hi << ".\n";
    }
}
inline bool confirm(const std::string& prompt) {
    while (true) {
        std::string s = toLower(trim(readLine(prompt + " (y/n): ")));
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        std::cout << "  Please type y or n.\n";
    }
}

}  // namespace Utils
