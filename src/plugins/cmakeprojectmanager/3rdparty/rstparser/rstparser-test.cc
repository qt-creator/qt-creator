/*
 reStructuredText parser tests.

 Copyright (c) 2012, Victor Zverovich
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

#include <gtest/gtest.h>

#include <stack>

#ifdef _WIN32
# include <crtdbg.h>
#endif

#include "rstparser.h"

namespace {

class TestHandler : public rst::ContentHandler {
 private:
  std::stack<std::string> tags_;
  std::string content_;

 public:
  const std::string &content() const { return content_; }

  void StartBlock(rst::BlockType type) {
    std::string tag;
    switch (type) {
    case rst::PARAGRAPH:
      tag = "p";
      break;
    case rst::LINE_BLOCK:
      tag = "lineblock";
      break;
    case rst::BLOCK_QUOTE:
      tag = "blockquote";
      break;
    case rst::BULLET_LIST:
      tag = "ul";
      break;
    case rst::LIST_ITEM:
      tag = "li";
      break;
    case rst::LITERAL_BLOCK:
      tag = "code";
      break;
    }
    content_ += "<" + tag + ">";
    tags_.push(tag);
  }

  void EndBlock() {
    content_ += "</" + tags_.top() + ">";
    tags_.pop();
  }

  void HandleText(const char *text, std::size_t size) {
    content_.append(text, size);
  }

  void HandleDirective(const char *type) {
    content_ += std::string("<") + type + " />";
  }
};

std::string Parse(const char *s) {
  TestHandler handler;
  rst::Parser parser(&handler);
  parser.Parse(s);
  return handler.content();
}
}

TEST(ParserTest, Paragraph) {
  EXPECT_EQ("<p>test</p>", Parse("test"));
  EXPECT_EQ("<p>test</p>", Parse("\ntest"));
  EXPECT_EQ("<p>.</p>", Parse("."));
  EXPECT_EQ("<p>..test</p>", Parse("..test"));
}

TEST(ParserTest, LineBlock) {
  EXPECT_EQ("<lineblock>test</lineblock>", Parse("| test"));
  EXPECT_EQ("<lineblock>  abc\ndef</lineblock>", Parse("|   abc\n| def"));
}

TEST(ParserTest, BlockQuote) {
  EXPECT_EQ("<blockquote>test</blockquote>", Parse(" test"));
}

TEST(ParserTest, PreserveInnerSpace) {
  EXPECT_EQ("<p>a  b</p>", Parse("a  b"));
}

TEST(ParserTest, ReplaceWhitespace) {
  EXPECT_EQ("<p>a       b</p>", Parse("a\tb"));
  EXPECT_EQ("<blockquote>a      b</blockquote>", Parse(" a\tb"));
  EXPECT_EQ("<p>a b</p>", Parse("a\vb"));
}

TEST(ParserTest, StripTrailingSpace) {
  EXPECT_EQ("<p>test</p>", Parse("test \t"));
}

TEST(ParserTest, MultiLineBlock) {
  EXPECT_EQ("<p>line 1\nline 2</p>", Parse("line 1\nline 2"));
}

TEST(ParserTest, UnindentBlock) {
  EXPECT_EQ("<blockquote>abc</blockquote><p>def</p>", Parse(" abc\ndef"));
}

TEST(ParserTest, BulletList) {
  EXPECT_EQ("<ul><li>item</li></ul>", Parse("* item"));
  EXPECT_EQ("<ul><li>abc\ndef</li></ul>", Parse("* abc\n  def"));
}

TEST(ParserTest, Literal) {
  EXPECT_EQ("<p>abc:</p><code>def</code>", Parse("abc::\n\n def"));
  EXPECT_EQ("<code>abc\ndef</code>", Parse("::\n\n abc\n def"));
  EXPECT_EQ("<p>abc\ndef</p>", Parse("::\n\nabc\ndef"));
  EXPECT_EQ("<p>::\nabc\ndef</p>", Parse("::\nabc\ndef"));
}

TEST(ParserTest, Comment) {
  EXPECT_EQ("", Parse(".."));
  EXPECT_EQ("", Parse("..\n"));
  EXPECT_EQ("", Parse(".. comment"));
  EXPECT_EQ("", Parse(".. comment:"));
}

TEST(ParserTest, Directive) {
  EXPECT_EQ("<test />", Parse(".. test::"));
  EXPECT_EQ("<test />", Parse("..  test::"));
  EXPECT_EQ("<test />", Parse("..\ttest::"));
}

int main(int argc, char **argv) {
#ifdef _WIN32
  // Disable message boxes on assertion failures.
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
