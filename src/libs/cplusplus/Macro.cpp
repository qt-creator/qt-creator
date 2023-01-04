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

#include "Macro.h"

using namespace CPlusPlus;

Macro::Macro()
    : _next(nullptr),
      _hashcode(0),
      _fileRevision(0),
      _line(0),
      _bytesOffset(0),
      _utf16charsOffset(0),
      _length(0),
      _state(0)
{ }

QString Macro::decoratedName() const
{
    QString text;
    if (f._hidden)
        text += QLatin1String("#undef ");
    else
        text += QLatin1String("#define ");
    text += QString::fromUtf8(_name.constData(), _name.size());
    if (f._functionLike) {
        text += QLatin1Char('(');
        bool first = true;
        for (const QByteArray &formal : std::as_const(_formals)) {
            if (! first)
                text += QLatin1String(", ");
            else
                first = false;
            if (formal != "__VA_ARGS__")
                text += QString::fromUtf8(formal.constData(), formal.size());
        }
        if (f._variadic)
            text += QLatin1String("...");
        text += QLatin1Char(')');
    }
    text += QLatin1Char(' ');
    return text;
}

QString Macro::toString() const
{
    QString text = decoratedName();
    text.append(QString::fromUtf8(_definitionText.constData(), _definitionText.size()));
    return text;
}

QString Macro::toStringWithLineBreaks() const
{
    return toString();
}
