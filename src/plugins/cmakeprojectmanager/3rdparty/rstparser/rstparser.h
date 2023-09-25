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

#ifndef RSTPARSER_H_
#define RSTPARSER_H_

#include <memory>
#include <string>
#include <vector>

namespace rst {

enum BlockType {
  PARAGRAPH,
  LINE_BLOCK,
  BLOCK_QUOTE,
  BULLET_LIST,
  LIST_ITEM,
  LITERAL_BLOCK
};

// Receive notification of the logical content of a document.
class ContentHandler {
 public:
  virtual ~ContentHandler();

  // Receives notification of the beginning of a text block.
  virtual void StartBlock(BlockType type) = 0;

  // Receives notification of the end of a text block.
  virtual void EndBlock() = 0;

  // Receives notification of text.
  virtual void HandleText(const char *text, std::size_t size) = 0;

  // Receives notification of a directive.
  virtual void HandleDirective(const char *type) = 0;
};

// A parser for a subset of reStructuredText.
class Parser {
 private:
  ContentHandler *handler_;
  const char *ptr_;

  // Skips whitespace.
  void SkipSpace();

  // Parses a directive type.
  std::string ParseDirectiveType();

  // Parses a paragraph.
  void ParseParagraph();

  // Changes the current block type sending notifications if necessary.
  void EnterBlock(rst::BlockType &prev_type, rst::BlockType type);

  // Parses a block of text.
  void ParseBlock(rst::BlockType type, rst::BlockType &prev_type, int indent);

  // Parses a line block.
  void ParseLineBlock(rst::BlockType &prev_type, int indent);

 public:
  explicit Parser(ContentHandler *h) : handler_(h), ptr_(0) {}

  // Parses a string containing reStructuredText and returns a document node.
  void Parse(const char *s);
};
}

#endif  // RSTPARSER_H_

