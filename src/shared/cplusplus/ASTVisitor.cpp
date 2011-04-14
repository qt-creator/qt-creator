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

#include "ASTVisitor.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Control.h"

using namespace CPlusPlus;

ASTVisitor::ASTVisitor(TranslationUnit *translationUnit)
    : _translationUnit(translationUnit)
{ }

ASTVisitor::~ASTVisitor()
{ }

void ASTVisitor::accept(AST *ast)
{ AST::accept(ast, this); }

Control *ASTVisitor::control() const
{
    if (_translationUnit)
        return _translationUnit->control();

    return 0;
}

TranslationUnit *ASTVisitor::translationUnit() const
{ return _translationUnit; }

void ASTVisitor::setTranslationUnit(TranslationUnit *translationUnit)
{ _translationUnit = translationUnit; }

unsigned ASTVisitor::tokenCount() const
{ return translationUnit()->tokenCount(); }

const Token &ASTVisitor::tokenAt(unsigned index) const
{ return translationUnit()->tokenAt(index); }

int ASTVisitor::tokenKind(unsigned index) const
{ return translationUnit()->tokenKind(index); }

const char *ASTVisitor::spell(unsigned index) const
{ return translationUnit()->spell(index); }

const Identifier *ASTVisitor::identifier(unsigned index) const
{ return translationUnit()->identifier(index); }

const Literal *ASTVisitor::literal(unsigned index) const
{ return translationUnit()->literal(index); }

const NumericLiteral *ASTVisitor::numericLiteral(unsigned index) const
{ return translationUnit()->numericLiteral(index); }

const StringLiteral *ASTVisitor::stringLiteral(unsigned index) const
{ return translationUnit()->stringLiteral(index); }

void ASTVisitor::getPosition(unsigned offset,
                             unsigned *line,
                             unsigned *column,
                             const StringLiteral **fileName) const
{ translationUnit()->getPosition(offset, line, column, fileName); }

void ASTVisitor::getTokenPosition(unsigned index,
                                  unsigned *line,
                                  unsigned *column,
                                  const StringLiteral **fileName) const
{ translationUnit()->getTokenPosition(index, line, column, fileName); }

void ASTVisitor::getTokenStartPosition(unsigned index, unsigned *line, unsigned *column) const
{ getPosition(tokenAt(index).begin(), line, column); }

void ASTVisitor::getTokenEndPosition(unsigned index, unsigned *line, unsigned *column) const
{ getPosition(tokenAt(index).end(), line, column); }


