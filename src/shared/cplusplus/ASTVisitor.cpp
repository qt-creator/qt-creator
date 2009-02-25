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

#include "ASTVisitor.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Control.h"

CPLUSPLUS_BEGIN_NAMESPACE

ASTVisitor::ASTVisitor(Control *control)
    : _control(control)
{ }

ASTVisitor::~ASTVisitor()
{ }

void ASTVisitor::accept(AST *ast)
{ AST::accept(ast, this); }

Control *ASTVisitor::control() const
{ return _control; }

TranslationUnit *ASTVisitor::translationUnit() const
{ return _control->translationUnit(); }

int ASTVisitor::tokenKind(unsigned index) const
{ return translationUnit()->tokenKind(index); }

const char *ASTVisitor::spell(unsigned index) const
{
    if (! index)
        return 0;

    return translationUnit()->tokenAt(index).spell();
}

Identifier *ASTVisitor::identifier(unsigned index) const
{ return translationUnit()->identifier(index); }

Literal *ASTVisitor::literal(unsigned index) const
{ return translationUnit()->literal(index); }

NumericLiteral *ASTVisitor::numericLiteral(unsigned index) const
{ return translationUnit()->numericLiteral(index); }

StringLiteral *ASTVisitor::stringLiteral(unsigned index) const
{ return translationUnit()->stringLiteral(index); }

void ASTVisitor::getTokenPosition(unsigned index,
                                  unsigned *line,
                                  unsigned *column,
                                  StringLiteral **fileName) const
{ translationUnit()->getTokenPosition(index, line, column, fileName); }

CPLUSPLUS_END_NAMESPACE
