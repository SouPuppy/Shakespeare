#pragma once
#include <cstddef>
#include <string>
namespace lingo::core {
class Document { public: void replace(std::string text); void set_cursor(size_t offset); const std::string& text() const; std::string current_sentence() const; std::string current_word() const; private: std::string text_; size_t cursor_ = 0; };
}
