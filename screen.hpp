#pragma once

#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

#include "ansi.hpp"

/// SCREEN

struct Cell {
  char character = ' ';
};

struct Screen {
  size_t width;
  size_t height;

  std::vector<Cell> cells;

  Screen(size_t screen_width, size_t screen_height)
      : width(screen_width), height(screen_height), cells(width * height) {}

  void clear() {
    for (auto& cell : cells) {
      cell.character = ' ';
    }
  }

  void set(size_t row, size_t column, char character) {
    if (row >= height || column >= width) return;
    cells[row * width + column].character = character;
  }

  const Cell& get(size_t row, size_t column) const {
    return cells[row * width + column];
  }
};

/// TERMINAL

enum class Key {
  Character,
  Left,
  Right,
  Up,
  Down,
  Backspace,
  Delete,
  Enter,
  Escape,
  Unknown,
};

struct InputEvent {
  Key key;
  char character = 0;
};

struct Terminal {
  std::vector<Cell> previous_cells;
  bool has_previous = false;

  bool start(size_t& width, size_t& height) {
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD output_mode = 0;
    if (!GetConsoleMode(output, &output_mode)) {
      if (!AllocConsole()) return false;
      output = GetStdHandle(STD_OUTPUT_HANDLE);
      if (!GetConsoleMode(output, &output_mode)) return false;
    }
    if (!SetConsoleMode(output, output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info)) return false;
    width = info.srWindow.Right - info.srWindow.Left + 1;
    height = info.srWindow.Bottom - info.srWindow.Top + 1;
    ansi::clear();
    ansi::hideCursor();
    previous_cells.resize(width * height);
    has_previous = false;
    return true;
  }

  void stop() {
    ansi::showCursor();
    ansi::clear();
  }

  void present(const Screen& screen, size_t cursor_row, size_t cursor_column) {
    for (size_t row = 0; row < screen.height; row++) {
      for (size_t column = 0; column < screen.width; column++) {
        const size_t index = row * screen.width + column;
        if (!has_previous || previous_cells[index].character != screen.cells[index].character) {
          ansi::cursor(row, column);
          std::cout << screen.cells[index].character;
          previous_cells[index] = screen.cells[index];
        }
      }
    }

    ansi::cursor(std::min(cursor_row, screen.height - 1), std::min(cursor_column, screen.width - 1));
    ansi::showCursor();
    std::cout.flush();
    has_previous = true;
  }

  InputEvent input() {
    int value = _getch();

    if (value == 27) return {Key::Escape};
    if (value == 13) return {Key::Enter};
    if (value == 8) return {Key::Backspace};

    if (value == 224) {
      value = _getch();
      if (value == 75) return {Key::Left};
      if (value == 77) return {Key::Right};
      if (value == 72) return {Key::Up};
      if (value == 80) return {Key::Down};
      if (value == 83) return {Key::Delete};
    }

    if (value >= 32 && value < 127) {
      return {Key::Character, static_cast<char>(value)};
    }

    return {Key::Unknown};
  }
};
