// UI.h - console look & feel: clear screen, colours, banners, tables, pauses.
// Presentation only. No library logic lives here.
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "Utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace UI {

const int WIDTH = 78;  // width of every banner / box

// ---------------- colours ----------------
enum Color { DEFAULT = 7, BLUE = 9, GREEN = 10, CYAN = 11, RED = 12, YELLOW = 14, WHITE = 15 };

inline void setColor(Color c) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)c);
#else
    const char* code = "0";
    switch (c) {
        case BLUE: code = "94"; break;
        case GREEN: code = "92"; break;
        case CYAN: code = "96"; break;
        case RED: code = "91"; break;
        case YELLOW: code = "93"; break;
        case WHITE: code = "97"; break;
        default: code = "0"; break;
    }
    std::cout << "\033[" << code << "m";
#endif
}
inline void resetColor() { setColor(DEFAULT); }

// ---------------- screen ----------------
inline void clearScreen() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(h, &info)) { std::system("cls"); return; }
    DWORD cells = (DWORD)info.dwSize.X * info.dwSize.Y, written;
    COORD home = {0, 0};
    FillConsoleOutputCharacterA(h, ' ', cells, home, &written);
    FillConsoleOutputAttribute(h, info.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(h, home);
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif
}

inline void pause(const std::string& text = "Press Enter to continue...") {
    setColor(YELLOW);
    std::cout << "\n  " << text << std::flush;
    resetColor();
    std::string dummy;
    std::getline(std::cin, dummy);
}

// ---------------- small helpers ----------------
inline std::string line(char c, int n = WIDTH) { return std::string(n, c); }

inline std::string center(const std::string& s, int w = WIDTH) {
    int n = (int)s.size();
    if (n >= w) return s;
    int left = (w - n) / 2;
    return std::string(left, ' ') + s + std::string(w - n - left, ' ');
}

// ---------------- banners and headings ----------------
// Big banner:   +----------+
//               |  TITLE   |
//               +----------+
inline void banner(const std::string& title, const std::string& subtitle = "") {
    setColor(CYAN);
    std::cout << "\n  +" << line('=', WIDTH - 2) << "+\n";
    std::cout << "  |";
    setColor(WHITE);
    std::cout << center(title, WIDTH - 2);
    setColor(CYAN);
    std::cout << "|\n";
    if (!subtitle.empty()) {
        std::cout << "  |";
        setColor(DEFAULT);
        std::cout << center(subtitle, WIDTH - 2);
        setColor(CYAN);
        std::cout << "|\n";
    }
    std::cout << "  +" << line('=', WIDTH - 2) << "+\n";
    resetColor();
}

// Screen heading: starts a fresh screen with a title bar (and who is logged in)
inline void screen(const std::string& title, const std::string& userInfo = "") {
    clearScreen();
    banner("LIBRARY MANAGEMENT SYSTEM");
    setColor(YELLOW);
    std::cout << "\n  >> " << title;
    resetColor();
    if (!userInfo.empty()) {
        setColor(DEFAULT);
        std::cout << "   [" << userInfo << "]";
    }
    std::cout << "\n  " << line('-', WIDTH - 2) << "\n";
    resetColor();
}

inline void section(const std::string& text) {
    setColor(CYAN);
    std::cout << "\n  " << text << "\n  " << line('-', (int)text.size()) << "\n";
    resetColor();
}

// ---------------- menu ----------------
inline void menuItem(const std::string& key, const std::string& text) {
    std::cout << "     ";
    setColor(YELLOW);
    std::cout << "[" << key << "]";
    resetColor();
    std::cout << " " << text << "\n";
}
inline void menuHeading(const std::string& text) {
    setColor(CYAN);
    std::cout << "\n   " << text << "\n";
    resetColor();
}
inline void menuItemPair(const std::string& k1, const std::string& t1, const std::string& k2,
                         const std::string& t2) {
    std::cout << "     ";
    setColor(YELLOW);
    std::cout << "[" << (k1.size() < 2 ? " " : "") << k1 << "]";
    resetColor();
    if (k2.empty()) { std::cout << " " << t1 << "\n"; return; }
    std::string a = " " + t1;
    a.resize(34, ' ');
    std::cout << a;
    setColor(YELLOW);
    std::cout << "[" << (k2.size() < 2 ? " " : "") << k2 << "]";
    resetColor();
    std::cout << " " << t2 << "\n";
}

// ---------------- messages ----------------
inline void success(const std::string& msg) {
    std::cout << "\n  ";
    setColor(GREEN);
    std::cout << "[ SUCCESS ] ";
    resetColor();
    std::cout << msg << "\n";
}
inline void error(const std::string& msg) {
    std::cout << "\n  ";
    setColor(RED);
    std::cout << "[ ERROR ]   ";
    resetColor();
    std::cout << msg << "\n";
}
inline void info(const std::string& msg) {
    std::cout << "\n  ";
    setColor(CYAN);
    std::cout << "[ INFO ]    ";
    resetColor();
    std::cout << msg << "\n";
}
inline void warning(const std::string& msg) {
    std::cout << "\n  ";
    setColor(YELLOW);
    std::cout << "[ NOTICE ]  ";
    resetColor();
    std::cout << msg << "\n";
}
inline void result(const Result& r) { r.ok ? success(r.msg) : error(r.msg); }

// ---------------- tables ----------------
// Usage: Table t({{"ID",6},{"Title",30}}); t.header(); t.row({"101","Clean Code"}); t.footer();
struct Column {
    std::string name;
    int width;  // inner width of the cell
};

class Table {
    std::vector<Column> cols;

    std::string border() const {
        std::string s = "  +";
        for (const auto& c : cols) s += std::string(c.width + 2, '-') + "+";
        return s;
    }

public:
    explicit Table(const std::vector<Column>& c) : cols(c) {}

    int totalWidth() const {
        int w = 3;
        for (const auto& c : cols) w += c.width + 3;
        return w;
    }

    void header() const {
        setColor(CYAN);
        std::cout << "\n" << border() << "\n  |";
        for (const auto& c : cols) {
            setColor(WHITE);
            std::string n = Utils::fit(c.name, (size_t)c.width);
            std::cout << " " << n << std::string(c.width - n.size(), ' ') << " ";
            setColor(CYAN);
            std::cout << "|";
        }
        std::cout << "\n" << border() << "\n";
        resetColor();
    }

    // colorIndex = which cell to colour (-1 = none)
    void row(const std::vector<std::string>& cells, int colorIndex = -1, Color color = DEFAULT) const {
        setColor(CYAN);
        std::cout << "  |";
        for (size_t i = 0; i < cols.size(); ++i) {
            std::string t = i < cells.size() ? Utils::fit(cells[i], (size_t)cols[i].width) : "";
            resetColor();
            std::cout << " ";
            if ((int)i == colorIndex) setColor(color);
            std::cout << t << std::string(cols[i].width - t.size(), ' ');
            resetColor();
            std::cout << " ";
            setColor(CYAN);
            std::cout << "|";
        }
        std::cout << "\n";
        resetColor();
    }

    void footer(const std::string& text = "") const {
        setColor(CYAN);
        std::cout << border() << "\n";
        resetColor();
        if (!text.empty()) std::cout << "  " << text << "\n";
    }
};

// Label / value pair used for "details" blocks
inline void field(const std::string& label, const std::string& value) {
    std::string l = label;
    l.resize(16, ' ');
    std::cout << "  ";
    setColor(CYAN);
    std::cout << l;
    resetColor();
    std::cout << ": " << value << "\n";
}

}  // namespace UI
