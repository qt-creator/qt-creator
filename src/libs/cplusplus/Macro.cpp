/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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

#include "Macro.h"

using namespace CPlusPlus;

Macro::Macro()
    : _next(0),
      _hashcode(0),
      _line(0),
      _state(0)
{ }

QString Macro::toString() const
{
    QString text;
    if (_hidden)
        text += QLatin1String("#undef ");
    else
        text += QLatin1String("#define ");
    text += QString::fromUtf8(_name.constData(), _name.size());
    if (_functionLike) {
        text += QLatin1Char('(');
        bool first = true;
        foreach (const QByteArray formal, _formals) {
            if (! first)
                text += QLatin1String(", ");
            else
                first = false;
            text += QString::fromUtf8(formal.constData(), formal.size());
        }
        if (_variadic)
            text += QLatin1String("...");
        text += QLatin1Char(')');
    }
    text += QLatin1Char(' ');
    text += QString::fromUtf8(_definition.constData(), _definition.size());
    return text;
}
