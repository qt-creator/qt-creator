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

#ifndef PP_MACRO_H
#define PP_MACRO_H

#include <CPlusPlusForwardDeclarations.h>

#include <QByteArray>
#include <QVector>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Macro
{
public:
    Macro();

    QByteArray name() const
    { return _name; }

    void setName(const QByteArray &name)
    { _name = name; }

    QByteArray definition() const
    { return _definition; }

    void setDefinition(const QByteArray &definition)
    { _definition = definition; }

    QVector<QByteArray> formals() const
    { return _formals; }

    void addFormal(const QByteArray &formal)
    { _formals.append(formal); }

    QByteArray fileName() const
    { return _fileName; }

    void setFileName(const QByteArray &fileName)
    { _fileName = fileName; }

    unsigned line() const
    { return _line; }

    void setLine(unsigned line)
    { _line = line; }

    bool isHidden() const
    { return _hidden; }

    void setHidden(bool isHidden)
    { _hidden = isHidden; }

    bool isFunctionLike() const
    { return _functionLike; }

    void setFunctionLike(bool isFunctionLike)
    { _functionLike = isFunctionLike; }

    bool isVariadic() const
    { return _variadic; }

    void setVariadic(bool isVariadic)
    { _variadic = isVariadic; }

    QString toString() const;

// ### private
    Macro *_next;
    unsigned _hashcode;

private:
    QByteArray _name;
    QByteArray _definition;
    QVector<QByteArray> _formals;
    QByteArray _fileName;
    unsigned _line;

    union
    {
        unsigned _state;

        struct
        {
            unsigned _hidden: 1;
            unsigned _functionLike: 1;
            unsigned _variadic: 1;
        };
    };
};

} // namespace CPlusPlus

#endif // PP_MACRO_H
