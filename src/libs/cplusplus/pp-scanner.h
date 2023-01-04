// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace CPlusPlus {

struct pp_skip_blanks
{
  int lines;

  pp_skip_blanks(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_whitespaces
{
  int lines;

  pp_skip_whitespaces(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_comment_or_divop
{
  int lines;

  pp_skip_comment_or_divop(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_identifier
{
  int lines;

  pp_skip_identifier(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_number
{
  int lines;

  pp_skip_number(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_string_literal
{
  int lines;

  pp_skip_string_literal(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_char_literal
{
  int lines;

  pp_skip_char_literal(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

struct pp_skip_argument
{
  pp_skip_identifier skip_number;
  pp_skip_identifier skip_identifier;
  pp_skip_string_literal skip_string_literal;
  pp_skip_char_literal skip_char_literal;
  pp_skip_comment_or_divop skip_comment_or_divop;
  int lines;

  pp_skip_argument(): lines(0) {}
  const char *operator () (const char *first, const char *last);
};

} // namespace CPlusPlus
