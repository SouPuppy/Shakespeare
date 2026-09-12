#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "screen.hpp"

/// BUFFER & PIECE TABLE

enum BufferType {
  origin,
  addition,
};

struct Piece {
  BufferType type;
  size_t start;
  size_t len;

  /* getters */
  size_t getEnd() const { return start + len; }
};

struct Buffer {
  std::string buffer_origin;
  std::string buffer_addition;
  std::vector<Piece> pieces;

  explicit Buffer(std::string text = {}) : buffer_origin(std::move(text)) {
    if (!buffer_origin.empty()) {
      pieces.push_back({origin, 0, buffer_origin.size()});
    }
  }

  size_t getLength() const {
    size_t len = 0;
    for (const auto& piece : pieces) len += piece.len;
    return len;
  }

  std::string getText() const {
    std::string text;
    for (const auto& piece : pieces) {
      const std::string& source = piece.type == origin ? buffer_origin : buffer_addition;
      text += source.substr(piece.start, piece.len);
    }
    return text;
  }

  void insert(size_t pos, const std::string& text) {
    if (text.empty()) return;
    pos = std::min(pos, getLength());
    size_t offset = 0;
    size_t index = pieces.size();
    for (size_t i = 0; i < pieces.size(); i++) {
      if (pos <= offset + pieces[i].len) { index = i; break; }
      offset += pieces[i].len;
    }
    size_t addition_start = buffer_addition.size();
    buffer_addition += text;
    if (index == pieces.size()) { pieces.push_back({addition, addition_start, text.size()}); return; }
    Piece old_piece = pieces[index];
    size_t local = pos - offset;
    std::vector<Piece> result;
    if (local > 0) result.push_back({old_piece.type, old_piece.start, local});
    result.push_back({addition, addition_start, text.size()});
    if (local < old_piece.len) result.push_back({old_piece.type, old_piece.start + local, old_piece.len - local});
    pieces.erase(pieces.begin() + index);
    pieces.insert(pieces.begin() + index, result.begin(), result.end());
  }

  void erase(size_t pos, size_t len = 1) {
    if (!len || pos >= getLength()) return;
    std::string text = getText();
    text.erase(pos, std::min(len, text.size() - pos));
    buffer_origin = std::move(text);
    buffer_addition.clear();
    pieces.clear();
    if (!buffer_origin.empty()) pieces.push_back({origin, 0, buffer_origin.size()});
  }
};

struct Document {
  Buffer buffer;
  size_t cursor = 0;

  void insert(const std::string& text) { buffer.insert(cursor, text); cursor += text.size(); }
  void backspace() { if (cursor > 0) { buffer.erase(cursor - 1); cursor--; } }
  void erase() { buffer.erase(cursor); }
};

struct Editor {
  Document document;
  size_t cursor_row = 0;
  size_t cursor_column = 0;

  void move(Key key) {
    if (key == Key::Left && cursor_column > 0) --cursor_column;
    if (key == Key::Right && cursor_column + 1 < Screen::width) ++cursor_column;
    if (key == Key::Up && cursor_row > 0) --cursor_row;
    if (key == Key::Down && cursor_row + 1 < Screen::height) ++cursor_row;
  }

  void render(Screen& screen) {
    screen.clear();
    size_t row = 0;
    size_t column = 0;
    size_t cursor = 0;
    const std::string text = document.buffer.getText();

    for (char character : text) {
      if (character == '\n') {
        row++;
        column = 0;
      } else if (row < Screen::height && column < Screen::width) {
        screen.set(row, column, character);
        column++;
      }
      cursor++;
    }

    cursor_row = std::min(row, Screen::height - 1);
    cursor_column = std::min(column, Screen::width - 1);
  }
};
