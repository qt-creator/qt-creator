/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
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

#ifndef CPLUSPLUS_PP_ENVIRONMENT_H
#define CPLUSPLUS_PP_ENVIRONMENT_H

#include "CPlusPlusForwardDeclarations.h"
#include "PPToken.h"

#include <QList>
#include <QByteArray>
#include <QString>

namespace CPlusPlus {

class Macro;

class CPLUSPLUS_EXPORT Environment
{
public:
    typedef Macro **iterator;

public:
    Environment();
    ~Environment();

    unsigned macroCount() const;
    Macro *macroAt(unsigned index) const;

    Macro *bind(const Macro &macro);
    Macro *remove(const ByteArrayRef &name);
    Macro *resolve(const ByteArrayRef &name) const;

    iterator firstMacro() const;
    iterator lastMacro() const;

    void reset();
    void addMacros(const QList<Macro> &macros);

    static bool isBuiltinMacro(const ByteArrayRef &name);
    void dump() const;

private:
    static unsigned hashCode(const QByteArray &s);
    static unsigned hashCode(const ByteArrayRef &s);
    void rehash();

public:
    QString currentFile;
    QByteArray currentFileUtf8;
    unsigned currentLine;
    bool hideNext;

private:
    Macro **_macros;
    int _allocated_macros;
    int _macro_count;
    Macro **_hash;
    int _hash_count;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_ENVIRONMENT_H
