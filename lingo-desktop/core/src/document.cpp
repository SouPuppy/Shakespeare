#include "lingo/core/document.hpp"
#include <algorithm>
#include <utility>
namespace lingo::core {
void Document::replace(std::string text) { text_ = std::move(text); cursor_ = std::min(cursor_, text_.size()); }
void Document::set_cursor(size_t offset) { cursor_ = std::min(offset, text_.size()); }
const std::string& Document::text() const { return text_; }
std::string Document::current_sentence() const {
  const bool at_end = cursor_ > 0 && cursor_ <= text_.size() && text_[cursor_ - 1] == '.';
  const size_t before = cursor_ == 0 ? std::string::npos : text_.rfind('.', at_end ? cursor_ - 2 : cursor_ - 1);
  const size_t end = text_.find('.', at_end ? cursor_ - 1 : cursor_);
  const size_t begin = before == std::string::npos ? 0 : before + 1;
  const size_t finish = end == std::string::npos ? text_.size() : end + 1;
  size_t first = begin;
  while (first < finish && (text_[first] == ' ' || text_[first] == '\n' || text_[first] == '\r' || text_[first] == '\t')) ++first;
  return text_.substr(first, finish - first);
}
}
