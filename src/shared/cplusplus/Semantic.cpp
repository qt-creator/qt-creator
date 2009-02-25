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

#include "Semantic.h"
#include "TranslationUnit.h"
#include "Control.h"
#include "Scope.h"
#include "Symbols.h"
#include "Token.h"
#include "CheckSpecifier.h"
#include "CheckDeclaration.h"
#include "CheckDeclarator.h"
#include "CheckStatement.h"
#include "CheckExpression.h"
#include "CheckName.h"

CPLUSPLUS_BEGIN_NAMESPACE

class Semantic::Data
{
public:
    Data(Semantic *semantic, Control *control)
        : semantic(semantic),
          control(control),
          visibility(Symbol::Public),
          methodKey(Function::NormalMethod),
          checkSpecifier(0),
          checkDeclaration(0),
          checkDeclarator(0),
          checkExpression(0),
          checkStatement(0),
          checkName(0)
    { }

    ~Data()
    {
        delete checkSpecifier;
        delete checkDeclaration;
        delete checkDeclarator;
        delete checkExpression;
        delete checkStatement;
        delete checkName;
    }

    Semantic *semantic;
    Control *control;
    int visibility;
    int methodKey;
    CheckSpecifier *checkSpecifier;
    CheckDeclaration *checkDeclaration;
    CheckDeclarator *checkDeclarator;
    CheckExpression *checkExpression;
    CheckStatement *checkStatement;
    CheckName *checkName;
};

Semantic::Semantic(Control *control)
{
    d = new Data(this, control);
    d->checkSpecifier = new CheckSpecifier(this);
    d->checkDeclaration = new CheckDeclaration(this);
    d->checkDeclarator = new CheckDeclarator(this);
    d->checkExpression = new CheckExpression(this);
    d->checkStatement = new CheckStatement(this);
    d->checkName = new CheckName(this);
}

Semantic::~Semantic()
{ delete d; }

Control *Semantic::control() const
{ return d->control; }

FullySpecifiedType Semantic::check(SpecifierAST *specifier, Scope *scope)
{ return d->checkSpecifier->check(specifier, scope); }

void Semantic::check(DeclarationAST *declaration, Scope *scope, Scope *templateParameters)
{ d->checkDeclaration->check(declaration, scope, templateParameters); }

FullySpecifiedType Semantic::check(DeclaratorAST *declarator, FullySpecifiedType type,
                                   Scope *scope, Name **name)
{ return d->checkDeclarator->check(declarator, type, scope, name); }

FullySpecifiedType Semantic::check(PtrOperatorAST *ptrOperators, FullySpecifiedType type,
                                   Scope *scope)
{ return d->checkDeclarator->check(ptrOperators, type, scope); }

FullySpecifiedType Semantic::check(ExpressionAST *expression, Scope *scope)
{ return d->checkExpression->check(expression, scope); }

void Semantic::check(StatementAST *statement, Scope *scope)
{ d->checkStatement->check(statement, scope); }

Name *Semantic::check(NameAST *name, Scope *scope)
{ return d->checkName->check(name, scope); }

Name *Semantic::check(NestedNameSpecifierAST *name, Scope *scope)
{ return d->checkName->check(name, scope); }

int Semantic::currentVisibility() const
{ return d->visibility; }

int Semantic::switchVisibility(int visibility)
{
    int previousVisibility = d->visibility;
    d->visibility = visibility;
    return previousVisibility;
}

int Semantic::currentMethodKey() const
{ return d->methodKey; }

int Semantic::switchMethodKey(int methodKey)
{
    int previousMethodKey = d->methodKey;
    d->methodKey = methodKey;
    return previousMethodKey;
}

int Semantic::visibilityForAccessSpecifier(int tokenKind) const
{
    switch (tokenKind) {
    case T_PUBLIC:
        return Symbol::Public;
    case T_PROTECTED:
        return Symbol::Protected;
    case T_PRIVATE:
        return Symbol::Private;
    case T_SIGNALS:
        return Symbol::Protected;
    default:
        return Symbol::Public;
    }
}

int Semantic::visibilityForClassKey(int tokenKind) const
{
    switch (tokenKind) {
    case T_CLASS:
        return Symbol::Private;
    case T_STRUCT:
    case T_UNION:
        return Symbol::Public;
    default:
        return Symbol::Public;
    }
}

CPLUSPLUS_END_NAMESPACE
