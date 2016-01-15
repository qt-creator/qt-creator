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

#include "pp-scanner.h"
#include "pp-cctype.h"

using namespace CPlusPlus;

const char *pp_skip_blanks::operator () (const char *first, const char *last)
{
    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        if (*first == '\\') {
            const char *begin = first;
            ++begin;

            if (begin != last && *begin == '\n')
                ++first;
            else
                break;
        } else if (*first == '\n' || !pp_isspace (*first))
            break;
    }

    return first;
}

const char *pp_skip_whitespaces::operator () (const char *first, const char *last)
{
    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        if (! pp_isspace (*first))
            break;
    }

    return first;
}

const char *pp_skip_comment_or_divop::operator () (const char *first, const char *last)
{
    enum {
        MAYBE_BEGIN,
        BEGIN,
        MAYBE_END,
        END,
        IN_COMMENT,
        IN_CXX_COMMENT
    } state (MAYBE_BEGIN);

    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        switch (state) {
        default:
            break;

        case MAYBE_BEGIN:
            if (*first != '/')
                return first;

            state = BEGIN;
            break;

        case BEGIN:
            if (*first == '*')
                state = IN_COMMENT;
            else if (*first == '/')
                state = IN_CXX_COMMENT;
            else
                return first;
            break;

        case IN_COMMENT:
            if (*first == '*')
                state = MAYBE_END;
            break;

        case IN_CXX_COMMENT:
            if (*first == '\n')
                return first;
            break;

        case MAYBE_END:
            if (*first == '/')
                state = END;
            else if (*first != '*')
                state = IN_COMMENT;
            break;

        case END:
            return first;
        }
    }

    return first;
}

const char *pp_skip_identifier::operator () (const char *first, const char *last)
{
    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        if (! pp_isalnum (*first) && *first != '_')
            break;
    }

    return first;
}

const char *pp_skip_number::operator () (const char *first, const char *last)
{
    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        if (! pp_isalnum (*first) && *first != '.')
            break;
    }

    return first;
}

const char *pp_skip_string_literal::operator () (const char *first, const char *last)
{
    enum {
        BEGIN,
        IN_STRING,
        QUOTE,
        END
    } state (BEGIN);

    lines = 0;

    for (; first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        switch (state)
        {
        default:
            break;

        case BEGIN:
            if (*first != '\"')
                return first;
            state = IN_STRING;
            break;

        case IN_STRING:
            if (! (*first != '\n'))
                return last;

            if (*first == '\"')
                state = END;
            else if (*first == '\\')
                state = QUOTE;
            break;

        case QUOTE:
            state = IN_STRING;
            break;

        case END:
            return first;
        }
    }

    return first;
}

const char *pp_skip_char_literal::operator () (const char *first, const char *last)
{
    enum {
        BEGIN,
        IN_STRING,
        QUOTE,
        END
    } state (BEGIN);

    lines = 0;

    for (; state != END && first != last; lines += (*first != '\n' ? 0 : 1), ++first) {
        switch (state)
        {
        default:
            break;

        case BEGIN:
            if (*first != '\'')
                return first;
            state = IN_STRING;
            break;

        case IN_STRING:
            if (! (*first != '\n'))
                return last;

            if (*first == '\'')
                state = END;
            else if (*first == '\\')
                state = QUOTE;
            break;

        case QUOTE:
            state = IN_STRING;
            break;
        }
    }

    return first;
}

const char *pp_skip_argument::operator () (const char *first, const char *last)
{
    int depth = 0;
    lines = 0;

    while (first != last) {
        if (!depth && (*first == ')' || *first == ',')) {
            break;
        } else if (*first == '(') {
            ++depth, ++first;
        } else if (*first == ')') {
            --depth, ++first;
        } else if (*first == '\"') {
            first = skip_string_literal (first, last);
            lines += skip_string_literal.lines;
        } else if (*first == '\'') {
            first = skip_char_literal (first, last);
            lines += skip_char_literal.lines;
        } else if (*first == '/') {
            first = skip_comment_or_divop (first, last);
            lines += skip_comment_or_divop.lines;
        } else if (pp_isalpha (*first) || *first == '_') {
            first = skip_identifier (first, last);
            lines += skip_identifier.lines;
        } else if (pp_isdigit (*first)) {
            first = skip_number (first, last);
            lines += skip_number.lines;
        } else if (*first == '\n') {
            ++first;
            ++lines;
        } else {
            ++first;
        }
    }

    return first;
}

