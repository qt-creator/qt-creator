/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
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

#ifndef CPLUSPLUS_PP_MACRO_EXPANDER_H
#define CPLUSPLUS_PP_MACRO_EXPANDER_H

#include "Macro.h"
#include "pp-scanner.h"
#include <QVector>
#include <QByteArray>

namespace CPlusPlus {

class Environment;
class Client;

struct pp_frame;

class MacroExpander
{
    Environment *env;
    pp_frame *frame;
    Client *client;
    unsigned start_offset;

    pp_skip_number skip_number;
    pp_skip_identifier skip_identifier;
    pp_skip_string_literal skip_string_literal;
    pp_skip_char_literal skip_char_literal;
    pp_skip_argument skip_argument;
    pp_skip_comment_or_divop skip_comment_or_divop;
    pp_skip_blanks skip_blanks;
    pp_skip_whitespaces skip_whitespaces;

    const QByteArray *resolve_formal(const QByteArray &name);

public:
    explicit MacroExpander(Environment *env, pp_frame *frame = 0, Client *client = 0, unsigned start_offset = 0);

    const char *operator()(const char *first, const char *last,
                             QByteArray *result);

    const char *operator()(const QByteArray &source,
                             QByteArray *result)
    { return operator()(source.constBegin(), source.constEnd(), result); }

    const char *expand(const char *first, const char *last,
                       QByteArray *result);

    const char *skip_argument_variadics(const QVector<QByteArray> &actuals,
                                         Macro *macro,
                                         const char *first, const char *last);

public: // attributes
    int lines;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_MACRO_EXPANDER_H

