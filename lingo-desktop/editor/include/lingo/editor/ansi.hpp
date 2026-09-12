#pragma once
#pragma once

#include <cstddef>
#include <iostream>

namespace ansi {

inline void hideCursor() { std::cout << "\x1b[?25l"; }
inline void showCursor() { std::cout << "\x1b[?25h"; }

inline void clear() {
  std::cout << "\x1b[2J\x1b[H";
}

inline void cursor(size_t row, size_t column) {
  std::cout << "\x1b[" << row + 1 << ';' << column + 1 << 'H';
}

}
