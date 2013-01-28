/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

const char *pp_skip_blanks::operator () (const char *__first, const char *__last)
{
    lines = 0;

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        if (*__first == '\\') {
            const char *__begin = __first;
            ++__begin;

            if (__begin != __last && *__begin == '\n')
                ++__first;
            else
                break;
        } else if (*__first == '\n' || !pp_isspace (*__first))
            break;
    }

    return __first;
}

const char *pp_skip_whitespaces::operator () (const char *__first, const char *__last)
{
    lines = 0;

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        if (! pp_isspace (*__first))
            break;
    }

    return __first;
}

const char *pp_skip_comment_or_divop::operator () (const char *__first, const char *__last)
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

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        switch (state) {
        default:
            break;

        case MAYBE_BEGIN:
            if (*__first != '/')
                return __first;

            state = BEGIN;
            break;

        case BEGIN:
            if (*__first == '*')
                state = IN_COMMENT;
            else if (*__first == '/')
                state = IN_CXX_COMMENT;
            else
                return __first;
            break;

        case IN_COMMENT:
            if (*__first == '*')
                state = MAYBE_END;
            break;

        case IN_CXX_COMMENT:
            if (*__first == '\n')
                return __first;
            break;

        case MAYBE_END:
            if (*__first == '/')
                state = END;
            else if (*__first != '*')
                state = IN_COMMENT;
            break;

        case END:
            return __first;
        }
    }

    return __first;
}

const char *pp_skip_identifier::operator () (const char *__first, const char *__last)
{
    lines = 0;

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        if (! pp_isalnum (*__first) && *__first != '_')
            break;
    }

    return __first;
}

const char *pp_skip_number::operator () (const char *__first, const char *__last)
{
    lines = 0;

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        if (! pp_isalnum (*__first) && *__first != '.')
            break;
    }

    return __first;
}

const char *pp_skip_string_literal::operator () (const char *__first, const char *__last)
{
    enum {
        BEGIN,
        IN_STRING,
        QUOTE,
        END
    } state (BEGIN);

    lines = 0;

    for (; __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        switch (state)
        {
        default:
            break;

        case BEGIN:
            if (*__first != '\"')
                return __first;
            state = IN_STRING;
            break;

        case IN_STRING:
            if (! (*__first != '\n'))
                return __last;

            if (*__first == '\"')
                state = END;
            else if (*__first == '\\')
                state = QUOTE;
            break;

        case QUOTE:
            state = IN_STRING;
            break;

        case END:
            return __first;
        }
    }

    return __first;
}

const char *pp_skip_char_literal::operator () (const char *__first, const char *__last)
{
    enum {
        BEGIN,
        IN_STRING,
        QUOTE,
        END
    } state (BEGIN);

    lines = 0;

    for (; state != END && __first != __last; lines += (*__first != '\n' ? 0 : 1), ++__first) {
        switch (state)
        {
        default:
            break;

        case BEGIN:
            if (*__first != '\'')
                return __first;
            state = IN_STRING;
            break;

        case IN_STRING:
            if (! (*__first != '\n'))
                return __last;

            if (*__first == '\'')
                state = END;
            else if (*__first == '\\')
                state = QUOTE;
            break;

        case QUOTE:
            state = IN_STRING;
            break;
        }
    }

    return __first;
}

const char *pp_skip_argument::operator () (const char *__first, const char *__last)
{
    int depth = 0;
    lines = 0;

    while (__first != __last) {
        if (!depth && (*__first == ')' || *__first == ','))
            break;
        else if (*__first == '(')
            ++depth, ++__first;
        else if (*__first == ')')
            --depth, ++__first;
        else if (*__first == '\"') {
            __first = skip_string_literal (__first, __last);
            lines += skip_string_literal.lines;
        } else if (*__first == '\'') {
            __first = skip_char_literal (__first, __last);
            lines += skip_char_literal.lines;
        } else if (*__first == '/') {
            __first = skip_comment_or_divop (__first, __last);
            lines += skip_comment_or_divop.lines;
        } else if (pp_isalpha (*__first) || *__first == '_') {
            __first = skip_identifier (__first, __last);
            lines += skip_identifier.lines;
        } else if (pp_isdigit (*__first)) {
            __first = skip_number (__first, __last);
            lines += skip_number.lines;
        } else if (*__first == '\n') {
            ++__first;
            ++lines;
        } else
            ++__first;
    }

    return __first;
}

