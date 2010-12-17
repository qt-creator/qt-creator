/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef CPLUSPLUS_SCOPE_H
#define CPLUSPLUS_SCOPE_H

#include "CPlusPlusForwardDeclarations.h"
#include "Symbol.h"
#include "Names.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Scope: public Symbol
{
public:
    Scope(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Scope();

    /// Adds a Symbol to this Scope.
    void addMember(Symbol *symbol);

    /// Returns true if this Scope is empty; otherwise returns false.
    bool isEmpty() const;

    /// Returns the number of symbols is in the scope.
    unsigned memberCount() const;

    /// Returns the Symbol at the given position.
    Symbol *memberAt(unsigned index) const;

    typedef Symbol **iterator;

    /// Returns the first Symbol in the scope.
    iterator firstMember() const;

    /// Returns the last Symbol in the scope.
    iterator lastMember() const;

    Symbol *find(const Identifier *id) const;
    Symbol *find(OperatorNameId::Kind operatorId) const;

    /// Set the start offset of the scope
    unsigned startOffset() const;
    void setStartOffset(unsigned offset);

    /// Set the end offset of the scope
    unsigned endOffset() const;
    void setEndOffset(unsigned offset);

    virtual const Scope *asScope() const
    { return this; }

    virtual Scope *asScope()
    { return this; }

private:
    SymbolTable *_members;
    unsigned _startOffset;
    unsigned _endOffset;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_SCOPE_H
