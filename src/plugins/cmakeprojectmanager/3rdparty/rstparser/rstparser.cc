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

#include <algorithm>
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
  if (!std::isalnum(*s) && *s != '|')
    return std::string();
  for (;;) {
    ++s;
    if (std::isalnum(*s))
      continue;
    switch (*s) {
    case '-': case '_': case '+': case ':': case '.': case '|':
      if (std::isalnum(s[1]) || (*s == '|' && IsSpace(s[1]))) {
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

  struct InlineTags {
    rst::BlockType type;
    std::size_t pos {};
    std::string text;
    std::string type_string;
  };
  std::vector<InlineTags> inline_tags;

  bool have_h1 = false;
  for (bool first = true; ; first = false) {
    const char *line_start = ptr_;
    if (!first) {
      // Check indentation.
      SkipSpace();
      const int new_indent = ptr_ - line_start;
      if (new_indent < indent)
        break;
      // Restore the indent
      if (new_indent > indent)
        std::advance(ptr_, indent - new_indent);

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
    if (!first && !have_h1)
      text.push_back('\n');
    enum {TAB_WIDTH = 8};

    // Used the sections mapping from https://docs.anaconda.com/restructuredtext/index.html
    struct {
      BlockType type;
      int count = 0;
      char c = 0;
    } hx[] = { {H1, 0, '=' }, {H2, 0, '='}, {H3, 0, '-'}, {H4, 0, '^'}, {H5, 0, '\"'}};

    for (const char *s = line_start; s != end; ++s) {
      char c = *s;
      if (c == '\t') {
        text.append("        ",
            TAB_WIDTH - ((indent + s - line_start) % TAB_WIDTH));
      } else if (IsSpace(c)) {
        text.push_back(' ');
      } else if (c == hx[0].c) {
        ++hx[0].count;
        ++hx[1].count;
      } else if (c == hx[2].c) {
        ++hx[2].count;
      } else if (c == hx[3].c) {
        ++hx[3].count;
      } else if (c == hx[4].c) {
        ++hx[4].count;
      } else if (c == '`') {
        std::string code_tag_text;
        if (ParseCode(s, end - s, code_tag_text)) {
          InlineTags code;
          code.type = rst::CODE;
          code.pos = text.size();
          code.text = code_tag_text;
          inline_tags.push_back(code);
          const int tag_size = 4;
          s = s + code_tag_text.size() + tag_size - 1;
        } else {
          text.push_back(*s);
        }
      } else if (c == ':') {
        std::string link_type;
        std::string link_text;
        if (ParseReferenceLink(s, end - s, link_type, link_text)) {
          InlineTags link;
          link.type = rst::REFERENCE_LINK;
          link.pos = text.size();
          link.text = link_text;
          link.type_string = link_type;
          inline_tags.push_back(link);
          const int tag_size = 4;
          s = s + link_type.size() + link_text.size() + tag_size - 1;
        } else {
          text.push_back(*s);
        }
      } else {
        text.push_back(*s);
      }
    }

    for (int i = 0; i < 5; ++i) {
      if (hx[i].count > 0 && hx[i].count == end - line_start) {
        // h1 and h2 have the same underline character
        // only if there was one ontop then is h1 otherwise h2
        if (i == 0 && first)
          have_h1 = true;
        if ((i == 0 && !have_h1) || (i == 1 && have_h1))
          continue;
        type = hx[i].type;
      }
    }

    if (*ptr_ == '\n')
      ++ptr_;
  }

  // Remove a trailing newline.
  if (!text.empty() && *text.rbegin() == '\n')
    text.resize(text.size() - 1);

  bool literal = type == PARAGRAPH && EndsWith(text, "::");
  if (!literal || text.size() != 2) {
    std::size_t size = text.size();
    if (size == 0 && inline_tags.size() == 0)
      return;

    if (literal)
      --size;
    EnterBlock(prev_type, type);
    handler_->StartBlock(type);

    if (inline_tags.size() == 0) {
      handler_->HandleText(text.c_str(), size);
    } else {
      std::size_t start = 0;
      for (const InlineTags &in : inline_tags) {
        if (in.pos > start)
          handler_->HandleText(text.c_str() + start, in.pos - start);
        if (in.type == rst::REFERENCE_LINK) {
          handler_->HandleReferenceLink(in.type_string, in.text);
        } else {
          handler_->StartBlock(in.type);
          handler_->HandleText(in.text.c_str(), in.text.size());
          handler_->EndBlock();
        }
        start = in.pos;
      }

      if (start < size)
        handler_->HandleText(text.c_str() + start, size - start);
    }

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

bool rst::Parser::ParseCode(const char *s, std::size_t size, std::string &code)
{
  // It requires at least four ticks ``text``
  if (s[0] != '`' || s[1] != '`')
    return false;

  if (size < 4)
    return false;

  std::size_t start_pos = 2;
  std::size_t end_pos = 0;
  for (std::size_t i = start_pos; i < size - 1; ++i) {
    if (s[i] == '`' && s[i + 1] == '`') {
      end_pos = i;
      break;
    }
  }

  if (end_pos == 0)
    return false;

  code.assign(s + start_pos, end_pos - start_pos);

  return true;
}

bool rst::Parser::ParseReferenceLink(const char *s, std::size_t size, std::string &type, std::string &text)
{
  // :type:`text`
  if (size < 4)
    return false;

  auto start_type_tag = s + 1;
  auto end_type_tag = std::find(start_type_tag, s + size, ':');
  if (end_type_tag == s + size)
    return false;

  type.assign(start_type_tag, end_type_tag - start_type_tag);

  if (*(end_type_tag + 1) != '`')
    return false;

  auto start_text_tag = end_type_tag + 2;
  auto end_text_tag = std::find(start_text_tag, s + size, '`');
  if (end_text_tag == s + size)
    return false;

  text.assign(start_text_tag, end_text_tag - start_text_tag);

  return true;
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

          const char* after_directive = ptr_;

          // Get the name of the directive
          std::string name;
          while (*ptr_ && *ptr_ != '\n') {
            c = *ptr_++;
            if (!IsSpace(c))
              name.push_back(c);
          }

          // Special case for ".. note::" which can start directly after the ::
          if (type == "note" && name.size() > 0) {
            ptr_ = after_directive;
            SkipSpace();
            handler_->HandleDirective(type, "");

            ParseBlock(BLOCK_QUOTE, prev_type, 0);
            break;
          }

          handler_->HandleDirective(type, name);
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
