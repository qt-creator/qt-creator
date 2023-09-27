/*
 A reStructuredText parser written in C++.

 Copyright (c) 2013, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "rstparser.h"

#include <cctype>
#include <cstring>

namespace {
inline bool IsSpace(char c) {
  switch (c) {
  case ' ': case '\t': case '\v': case '\f':
    return true;
  }
  return false;
}

// Returns true if s ends with string end.
bool EndsWith(const std::string &s, const char *end) {
  std::size_t size = s.size(), end_size = std::strlen(end);
  return size >= end_size ? std::strcmp(&s[size - end_size], end) == 0 : false;
}
}

rst::ContentHandler::~ContentHandler() {}

void rst::Parser::SkipSpace() {
  while (IsSpace(*ptr_))
    ++ptr_;
}

std::string rst::Parser::ParseDirectiveType() {
  const char *s = ptr_;
  if (!std::isalnum(*s))
    return std::string();
  for (;;) {
    ++s;
    if (std::isalnum(*s))
      continue;
    switch (*s) {
    case '-': case '_': case '+': case ':': case '.':
      if (std::isalnum(s[1])) {
        ++s;
        continue;
      }
      // Fall through.
    }
    break;
  }
  std::string type;
  if (s != ptr_)
    type.assign(ptr_, s);
  ptr_ = s;
  return type;
}

void rst::Parser::EnterBlock(rst::BlockType &prev_type, rst::BlockType type) {
  if (type == prev_type)
    return;
  if (prev_type == LIST_ITEM)
    handler_->EndBlock();
  if (type == LIST_ITEM)
    handler_->StartBlock(BULLET_LIST);
  prev_type = type;
}

void rst::Parser::ParseBlock(
    rst::BlockType type, rst::BlockType &prev_type, int indent) {
  std::string text;
  for (bool first = true; ; first = false) {
    const char *line_start = ptr_;
    if (!first) {
      // Check indentation.
      SkipSpace();
      if (ptr_ - line_start != indent)
        break;
      if (*ptr_ == '\n') {
        ++ptr_;
        break;  // Empty line ends the block.
      }
      if (!*ptr_)
        break;  // End of input.
    }
    // Strip indentation.
    line_start = ptr_;

    // Find the end of the line.
    while (*ptr_ && *ptr_ != '\n')
      ++ptr_;

    // Strip whitespace at the end of the line.
    const char *end = ptr_;
    while (end != line_start && IsSpace(end[-1]))
      --end;

    // Copy text converting all whitespace characters to spaces.
    text.reserve(end - line_start + 1);
    if (!first)
      text.push_back('\n');
    enum {TAB_WIDTH = 8};
    for (const char *s = line_start; s != end; ++s) {
      char c = *s;
      if (c == '\t') {
        text.append("        ",
            TAB_WIDTH - ((indent + s - line_start) % TAB_WIDTH));
      } else if (IsSpace(c)) {
        text.push_back(' ');
      } else {
        text.push_back(*s);
      }
    }
    if (*ptr_ == '\n')
      ++ptr_;
  }

  // Remove a trailing newline.
  if (*text.rbegin() == '\n')
    text.resize(text.size() - 1);

  bool literal = type == PARAGRAPH && EndsWith(text, "::");
  if (!literal || text.size() != 2) {
    std::size_t size = text.size();
    if (literal)
      --size;
    EnterBlock(prev_type, type);
    handler_->StartBlock(type);
    handler_->HandleText(text.c_str(), size);
    handler_->EndBlock();
  }
  if (literal) {
    // Parse a literal block.
    const char *line_start = ptr_;
    SkipSpace();
    int new_indent = static_cast<int>(ptr_ - line_start);
    if (new_indent > indent)
      ParseBlock(LITERAL_BLOCK, prev_type, new_indent);
  }
}

void rst::Parser::ParseLineBlock(rst::BlockType &prev_type, int indent) {
  std::string text;
  for (bool first = true; ; first = false) {
    const char *line_start = ptr_;
    if (!first) {
      // Check indentation.
      SkipSpace();
      if (*ptr_ != '|' || !IsSpace(ptr_[1]) || ptr_ - line_start != indent)
        break;
      ptr_ += 2;
      if (!*ptr_)
        break;  // End of input.
    }
    // Strip indentation.
    line_start = ptr_;

    // Find the end of the line.
    while (*ptr_ && *ptr_ != '\n')
      ++ptr_;
    if (*ptr_ == '\n')
      ++ptr_;
    text.append(line_start, ptr_);
  }

  EnterBlock(prev_type, rst::LINE_BLOCK);
  handler_->StartBlock(rst::LINE_BLOCK);
  handler_->HandleText(text.c_str(), text.size());
  handler_->EndBlock();
}

void rst::Parser::Parse(const char *s) {
  BlockType prev_type = PARAGRAPH;
  ptr_ = s;
  while (*ptr_) {
    // Skip whitespace and empty lines.
    const char *line_start = ptr_;
    SkipSpace();
    if (*ptr_ == '\n') {
      ++ptr_;
      continue;
    }
    switch (*ptr_) {
    case '.':
      if (ptr_[1] == '.') {
        char c = ptr_[2];
        if (!IsSpace(c) && c != '\n' && c)
          break;
        // Parse a directive or a comment.
        ptr_ += 2;
        SkipSpace();
        std::string type = ParseDirectiveType();
        if (!type.empty() && ptr_[0] == ':' && ptr_[1] == ':') {
          ptr_ += 2;
          handler_->HandleDirective(type.c_str());
        }
        // Skip everything till the end of the line.
        while (*ptr_ && *ptr_ != '\n')
          ++ptr_;
        if (*ptr_ == '\n')
          ++ptr_;
        continue;
      }
      break;
    case '*': case '+': case '-':
      if (IsSpace(ptr_[1])) {
        // Parse a bullet list item.
        ptr_ += 2;
        ParseBlock(LIST_ITEM, prev_type, static_cast<int>(ptr_ - line_start));
        continue;
      }
      break;
    case '|':
      if (IsSpace(ptr_[1])) {
        // Parse a line block.
        int indent = static_cast<int>(ptr_ - line_start);
        ptr_ += 2;
        ParseLineBlock(prev_type, indent);
        continue;
      }
      break;
    }
    ParseBlock(std::isspace(line_start[0]) ? BLOCK_QUOTE : PARAGRAPH,
        prev_type, static_cast<int>(ptr_ - line_start));
  }
  EnterBlock(prev_type, PARAGRAPH);
}
