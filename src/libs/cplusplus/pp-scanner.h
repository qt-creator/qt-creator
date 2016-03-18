/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
