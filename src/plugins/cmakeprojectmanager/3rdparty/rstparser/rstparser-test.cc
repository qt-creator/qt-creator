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
    case rst::REFERENCE_LINK:
      // not used, HandleReferenceLink is used instead
      break;
    case rst::H1:
      tag = "h1";
      break;
    case rst::H2:
      tag = "h2";
      break;
    case rst::H3:
      tag = "h3";
      break;
    case rst::H4:
      tag = "h4";
      break;
    case rst::H5:
      tag = "h5";
      break;
    case rst::CODE:
      tag = "code";
      break;
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

  void HandleDirective(const std::string &type, const std::string &name) {
    content_ += std::string("<div class=\"") + name + "\">" + type + "</div>";
  }

  void HandleReferenceLink(const std::string &type, const std::string &text) {
    content_ += std::string("<a href=\"#") + type + "\">" + text + "</a>";
  }
};

std::string Parse(const char *s) {
  TestHandler handler;
  rst::Parser parser(&handler);
  parser.Parse(s);
  return handler.content();
}
}

TEST(ParserTest, HX) {
  EXPECT_EQ("<h1>test</h1>", Parse("====\ntest\n===="));
  EXPECT_EQ("<h2>test</h2>", Parse("test\n===="));
  EXPECT_EQ("<h3>test</h3>", Parse("test\n----"));
  EXPECT_EQ("<h4>test</h4>", Parse("test\n^^^^"));
  EXPECT_EQ("<h5>test</h5>", Parse("test\n\"\"\"\""));
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

TEST(ParserTest, InlineCode) {
  EXPECT_EQ("<p><code>code</code></p>", Parse("``code``"));
  EXPECT_EQ("<p>`code``</p>", Parse("`code``"));
  EXPECT_EQ("<p>some <code>code</code></p>", Parse("some ``code``"));
  EXPECT_EQ("<p><code>code</code> some</p>", Parse("``code`` some"));
  EXPECT_EQ("<p>some <code>code</code> and more</p>", Parse("some ``code`` and more"));
}

TEST(ParserTest, Comment) {
  EXPECT_EQ("", Parse(".."));
  EXPECT_EQ("", Parse("..\n"));
  EXPECT_EQ("", Parse(".. comment"));
  EXPECT_EQ("", Parse(".. comment:"));
}

TEST(ParserTest, Directive) {
  EXPECT_EQ("<div class=\"\">test</div>", Parse(".. test::"));
  EXPECT_EQ("<div class=\"name\">test</div>", Parse(".. test:: name"));
  EXPECT_EQ("<div class=\"\">test</div>", Parse("..  test::"));
  EXPECT_EQ("<div class=\"\">test</div>", Parse("..\ttest::"));

  EXPECT_EQ("<div class=\"to-text\">|from-text| replace</div>", Parse(".. |from-text| replace:: to-text"));

  std::string rst =
R"(.. code-block:: c++
  int main() {
    if (false)
        return 1;
    return 0;
  })";

  std::string html =
R"(<div class="c++">code-block</div><blockquote>int main() {
  if (false)
      return 1;
  return 0;
}</blockquote>)";

  EXPECT_EQ(html, Parse(rst.c_str()));

  rst =
R"(.. note:: This is a cool
             note. Such a cool note.)";

  html =
R"(<div class="">note</div><blockquote>This is a cool
             note. Such a cool note.</blockquote>)";

  EXPECT_EQ(html, Parse(rst.c_str()));
}

TEST(ParserTest, ReferenceLinks) {
  EXPECT_EQ("<p><a href=\"#ref\">info</a></p>", Parse(":ref:`info`"));
  EXPECT_EQ("<p>some <a href=\"#ref\">info</a></p>", Parse("some :ref:`info`"));
  EXPECT_EQ("<p>some <a href=\"#ref\">info</a> and more</p>", Parse("some :ref:`info` and more"));
  EXPECT_EQ("<p><a href=\"#ref\">info</a>.</p>", Parse(":ref:`info`."));
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
