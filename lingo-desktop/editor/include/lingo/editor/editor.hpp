#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <sstream>
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

  void insert(size_t pos, const std::string& text, bool coalesce = false) {
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
    if (coalesce && index < pieces.size()) {
      Piece& piece = pieces[index];
      if (pos == offset + piece.len && piece.type == addition && piece.getEnd() == addition_start) {
        piece.len += text.size();
        log_change("insert/coalesced", pos, text);
        return;
      }
    }
    if (index == pieces.size()) {
      if (coalesce && !pieces.empty() && pieces.back().type == addition && pieces.back().getEnd() == addition_start) {
        pieces.back().len += text.size();
        log_change("insert/coalesced", pos, text);
        return;
      }
      pieces.push_back({addition, addition_start, text.size()});
      log_change("insert", pos, text);
      return;
    }
    Piece old_piece = pieces[index];
    size_t local = pos - offset;
    std::vector<Piece> result;
    if (local > 0) result.push_back({old_piece.type, old_piece.start, local});
    result.push_back({addition, addition_start, text.size()});
    if (local < old_piece.len) result.push_back({old_piece.type, old_piece.start + local, old_piece.len - local});
    pieces.erase(pieces.begin() + index);
    pieces.insert(pieces.begin() + index, result.begin(), result.end());
    log_change("insert", pos, text);
  }

  void erase(size_t pos, size_t len = 1) {
    if (!len || pos >= getLength()) return;
    len = std::min(len, getLength() - pos);
    const std::string removed = getText().substr(pos, len);
    const size_t end = pos + len;
    size_t offset = 0;
    std::vector<Piece> result;
    for (const auto& piece : pieces) {
      const size_t piece_end = offset + piece.len;
      if (piece_end <= pos || offset >= end) {
        result.push_back(piece);
      } else {
        if (offset < pos) result.push_back({piece.type, piece.start, pos - offset});
        if (piece_end > end) result.push_back({piece.type, piece.start + end - offset, piece_end - end});
      }
      offset = piece_end;
    }
    pieces = std::move(result);
    log_change("erase", pos, removed);
  }

  static std::string escaped(const std::string& text) {
    std::string result;
    for (char character : text) {
      if (character == '\n') result += "\\n";
      else if (character == '\r') result += "\\r";
      else if (character == '\t') result += "\\t";
      else if (character == '\\') result += "\\\\";
      else if (character == '"') result += "\\\"";
      else result += character;
    }
    return result;
  }

  void log_change(const char* operation, size_t pos, const std::string& detail) const {
    std::ostringstream message;
    message << "[buffer] " << operation
            << " pos=" << pos
            << " detail=\"" << escaped(detail) << "\""
            << " buffer_size=" << getLength()
            << " addition_buffer_size=" << buffer_addition.size()
            << " piece_entries=" << pieces.size()
            << " text=\"" << escaped(getText()) << "\"\n";
    OutputDebugStringA(message.str().c_str());
  }
};

struct Document {
  Buffer buffer;
  size_t cursor = 0;
  bool typing = false;
  size_t next_cursor = 0;

  void break_input() { typing = false; }

  void insert(const std::string& text) {
    if (text.empty()) return;
    const bool ordinary = text.find_first_of(" \n\r\t") == std::string::npos;
    buffer.insert(cursor, text, ordinary && typing && cursor == next_cursor);
    cursor += text.size();
    typing = ordinary;
    next_cursor = cursor;
    log_operation(text == " " ? "space" : text == "\n" ? "enter" : "insert", text);
  }
  void backspace() {
    break_input();
    if (cursor > 0) {
      buffer.erase(cursor - 1);
      cursor--;
    }
    log_operation("backspace", "");
  }
  void erase() {
    break_input();
    buffer.erase(cursor);
    log_operation("delete", "");
  }

  void log_operation(const char* operation, const std::string& detail) const {
    std::ostringstream message;
    message << "[input] " << operation << " detail=\"" << Buffer::escaped(detail)
            << "\" cursor=" << cursor
            << " buffer_size=" << buffer.getLength()
            << " addition_buffer_size=" << buffer.buffer_addition.size()
            << " piece_entries=" << buffer.pieces.size() << "\n";
    OutputDebugStringA(message.str().c_str());
  }
};

struct Editor {
  Document document;
  size_t cursor_row = 0;
  size_t cursor_column = 0;

  void move(Key key) {
    document.break_input();
    const std::string text = document.buffer.getText();
    if (key == Key::Left && document.cursor > 0) --document.cursor;
    if (key == Key::Right && document.cursor < text.size()) ++document.cursor;
    if (key == Key::Up || key == Key::Down) {
      size_t line_start = document.cursor;
      while (line_start > 0 && text[line_start - 1] != '\n') --line_start;
      const size_t column = document.cursor - line_start;
      size_t target = line_start;
      if (key == Key::Up) {
        if (line_start == 0) { document.log_operation("up", ""); return; }
        size_t previous_start = line_start - 1;
        while (previous_start > 0 && text[previous_start - 1] != '\n') --previous_start;
        target = previous_start + std::min(column, line_start - previous_start - 1);
      } else {
        const size_t next = text.find('\n', document.cursor);
        if (next == std::string::npos) { document.log_operation("down", ""); return; }
        const size_t next_end = text.find('\n', next + 1);
        target = next + 1 + std::min(column, (next_end == std::string::npos ? text.size() : next_end) - next - 1);
      }
      document.cursor = target;
    }
    document.log_operation(key == Key::Left ? "left" : key == Key::Right ? "right" : key == Key::Up ? "up" : "down", "");
  }

  void render(Screen& screen) {
    screen.clear();
    size_t row = 0;
    size_t column = 0;
    const std::string text = document.buffer.getText();

    const size_t content_height = screen.height - 1;
    size_t index = 0;
    for (char character : text) {
      if (index == document.cursor) {
        cursor_row = std::min(row, content_height - 1);
        cursor_column = std::min(column, screen.width - 1);
      }
      if (character == '\n') {
        row++;
        column = 0;
      } else if (row < content_height && column < screen.width) {
        screen.set(row, column, character);
        column++;
      }
      index++;
    }

    if (document.cursor == text.size()) {
      cursor_row = std::min(row, content_height - 1);
      cursor_column = std::min(column, screen.width - 1);
    }
    const std::string status = "line " + std::to_string(cursor_row + 1) + " col " + std::to_string(cursor_column + 1);
    constexpr size_t status_right_padding = 2;
    const size_t status_start = status.size() + status_right_padding < screen.width
                                    ? screen.width - status.size() - status_right_padding
                                    : 0;
    for (size_t status_column = 0; status_column < status.size() && status_start + status_column < screen.width; ++status_column) screen.set(screen.height - 1, status_start + status_column, status[status_column]);
  }
};
