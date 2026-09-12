#include <iostream>
#include <string>
#include <vector>

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

  /* functions */
  void insert(size_t pos, std::string str);

  // Total text length
  size_t getLength() const {
    size_t len = 0;
    for (auto& piece : pieces) {
      len += piece.len;
    }
    return len;
  }

  int find(size_t pos) {
    size_t offset = 0;

    for (size_t i = 0; i < pieces.size(); i++) {
      if (pos >= offset && pos <= offset + pieces[i].len)
        return i;

      offset += pieces[i].len;
    }

    return -1;
  }

  void dump() {
    for (auto& piece : pieces) {
      if (piece.type == BufferType::origin) {
        std::cout << buffer_origin.substr(piece.start, piece.len);
      } else if (piece.type == BufferType::addition) {
        std::cout << buffer_addition.substr(piece.start, piece.len);
      } 
    }

    std::cout << "\n";
  }

};

void Buffer::insert(size_t pos, std::string str) {

  if (pieces.empty()) {
    size_t start = buffer_addition.size();

    buffer_addition += str;

    pieces.push_back({
      addition,
      start,
      str.size()
    });

    return;
  }

  int index = find(pos);

  if (index == -1)
    return;

  Piece old_piece = pieces[index];

  size_t piece_begin = 0;

  for (int i = 0; i < index; i++)
    piece_begin += pieces[i].len;

  size_t local = pos - piece_begin;

  size_t add_start = buffer_addition.size();

  buffer_addition += str;

  Piece new_piece {
    addition,
    add_start,
    str.size()
  };

  std::vector<Piece> result;

  if (local > 0) {
    result.push_back({
      old_piece.type,
      old_piece.start,
      local
    });
  }

  result.push_back(new_piece);

  if (local < old_piece.len) {
    result.push_back({
      old_piece.type,
      old_piece.start + local,
      old_piece.len - local
    });
  }

  pieces.erase(pieces.begin() + index);

  pieces.insert(
    pieces.begin() + index,
    result.begin(),
    result.end()
  );
}

struct Document {
  Buffer buffer;

  void insert(size_t pos, std::string str) {
    buffer.insert(pos, str);
  }

  void dump() {
    buffer.dump();
  }
};

int main() {
  Document doc;

  doc.insert(0, "Hello World");
  doc.dump();

  doc.insert(6, "beautiful ");
  doc.dump();

  return 0;
}