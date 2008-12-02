/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>
  Copyright 2006 Hamish Rodda <rodda@kde.org>

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

#include <QtCore/QDebug>
#include "pp-scanner.h"

void pp_skip_blanks::operator()(Stream& input, Stream& output)
{
  while (!input.atEnd()) {
    if (input == '\\') {
      ++input;
      if (input != '\n') {
        --input;
        return;

      } else {
        ++input;
        continue;
      }
    }

    if (input == '\n' || !input.current().isSpace())
      return;

    output << input;
    ++input;
  }
}

void pp_skip_comment_or_divop::operator()(Stream& input, Stream& output, bool outputText)
{
  enum {
    MAYBE_BEGIN,
    BEGIN,
    MAYBE_END,
    END,
    IN_COMMENT,
    IN_CXX_COMMENT
  } state (MAYBE_BEGIN);

  while (!input.atEnd()) {
    switch (state) {
      case MAYBE_BEGIN:
        if (input != '/')
          return;

        state = BEGIN;
        break;

      case BEGIN:
        if (input == '*')
          state = IN_COMMENT;
        else if (input == '/')
          state = IN_CXX_COMMENT;
        else
          return;
        break;

      case IN_COMMENT:
        if (input == '*')
          state = MAYBE_END;
        break;

      case IN_CXX_COMMENT:
        if (input == '\n')
          return;
        break;

      case MAYBE_END:
        if (input == '/')
          state = END;
        else if (input != '*')
          state = IN_COMMENT;
        break;

      case END:
        return;
    }

    if (outputText)
      output << input;
    else if (input == '\n')
      output << '\n';
    else
      output << ' ';
    ++input;
  }
}

QString pp_skip_identifier::operator()(Stream& input)
{
  QString identifier;

  while (!input.atEnd()) {
    if (!input.current().isLetterOrNumber() && input != '_')
        break;

    identifier.append(input);
    ++input;
  }

  return identifier;
}

void pp_skip_number::operator()(Stream& input, Stream& output)
{
  while (!input.atEnd()) {
    if (!input.current().isLetterOrNumber() && input != '_')
        return;

    output << input;
    ++input;
  }
}

void pp_skip_string_literal::operator()(Stream& input, Stream& output)
{
  enum {
    BEGIN,
    IN_STRING,
    QUOTE,
    END
  } state (BEGIN);

  while (!input.atEnd()) {
    switch (state) {
      case BEGIN:
        if (input != '\"')
          return;
        state = IN_STRING;
        break;

      case IN_STRING:
        Q_ASSERT(input != '\n');

        if (input == '\"')
          state = END;
        else if (input == '\\')
          state = QUOTE;
        break;

      case QUOTE:
        state = IN_STRING;
        break;

      case END:
        return;
    }

    output << input;
    ++input;
  }
}

void pp_skip_char_literal::operator()(Stream& input, Stream& output)
{
  enum {
    BEGIN,
    IN_STRING,
    QUOTE,
    END
  } state (BEGIN);

  while (!input.atEnd()) {
    if (state == END)
      break;

    switch (state) {
      case BEGIN:
        if (input != '\'')
          return;
        state = IN_STRING;
        break;

      case IN_STRING:
        Q_ASSERT(input != '\n');

        if (input == '\'')
          state = END;
        else if (input == '\\')
          state = QUOTE;
        break;

      case QUOTE:
        state = IN_STRING;
        break;

      default:
        Q_ASSERT(0);
        break;
    }

    output << input;
    ++input;
  }
}

void pp_skip_argument::operator()(Stream& input, Stream& output)
{
  int depth = 0;

  while (!input.atEnd()) {
    if (!depth && (input == ')' || input == ',')) {
      return;

    } else if (input == '(') {
      ++depth;

    } else if (input == ')') {
      --depth;

    } else if (input == '\"') {
      skip_string_literal(input, output);
      continue;

    } else if (input == '\'') {
      skip_char_literal (input, output);
      continue;

    } else if (input == '/') {
      skip_comment_or_divop (input, output);
      continue;

    } else if (input.current().isLetter() || input == '_') {
      output << skip_identifier(input);
      continue;

    } else if (input.current().isNumber()) {
      output << skip_number(input);
      continue;

    }

    output << input;
    ++input;
  }

  return;
}
