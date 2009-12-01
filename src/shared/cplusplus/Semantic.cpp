/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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

using namespace CPlusPlus;

class Semantic::Data
{
public:
    Data(Semantic *semantic, TranslationUnit *translationUnit)
        : semantic(semantic),
          translationUnit(translationUnit),
          control(translationUnit->control()),
          skipFunctionBodies(false),
          visibility(Symbol::Public),
          ojbcVisibility(Symbol::Protected),
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
    TranslationUnit *translationUnit;
    Control *control;
    bool skipFunctionBodies;
    int visibility;
    int ojbcVisibility;
    int methodKey;
    CheckSpecifier *checkSpecifier;
    CheckDeclaration *checkDeclaration;
    CheckDeclarator *checkDeclarator;
    CheckExpression *checkExpression;
    CheckStatement *checkStatement;
    CheckName *checkName;
};

Semantic::Semantic(TranslationUnit *translationUnit)
{
    d = new Data(this, translationUnit);
    d->checkSpecifier = new CheckSpecifier(this);
    d->checkDeclaration = new CheckDeclaration(this);
    d->checkDeclarator = new CheckDeclarator(this);
    d->checkExpression = new CheckExpression(this);
    d->checkStatement = new CheckStatement(this);
    d->checkName = new CheckName(this);
}

Semantic::~Semantic()
{ delete d; }

TranslationUnit *Semantic::translationUnit() const
{ return d->translationUnit; }

Control *Semantic::control() const
{ return d->control; }

FullySpecifiedType Semantic::check(SpecifierListAST *specifier, Scope *scope)
{ return d->checkSpecifier->check(specifier, scope); }

void Semantic::check(DeclarationAST *declaration, Scope *scope, TemplateParameters *templateParameters)
{ d->checkDeclaration->check(declaration, scope, templateParameters); }

FullySpecifiedType Semantic::check(DeclaratorAST *declarator, const FullySpecifiedType &type,
                                   Scope *scope, const Name **name)
{ return d->checkDeclarator->check(declarator, type, scope, name); }

FullySpecifiedType Semantic::check(PtrOperatorListAST *ptrOperators, const FullySpecifiedType &type,
                                   Scope *scope)
{ return d->checkDeclarator->check(ptrOperators, type, scope); }

FullySpecifiedType Semantic::check(ObjCMethodPrototypeAST *methodPrototype, Scope *scope)
{ return d->checkDeclarator->check(methodPrototype, scope); }

FullySpecifiedType Semantic::check(ObjCTypeNameAST *typeName, Scope *scope)
{ return d->checkSpecifier->check(typeName, scope); }

void Semantic::check(ObjCMessageArgumentDeclarationAST *arg, Scope *scope)
{ return d->checkName->check(arg, scope); }

FullySpecifiedType Semantic::check(ExpressionAST *expression, Scope *scope)
{ return d->checkExpression->check(expression, scope); }

void Semantic::check(StatementAST *statement, Scope *scope)
{ d->checkStatement->check(statement, scope); }

const Name *Semantic::check(NameAST *name, Scope *scope)
{ return d->checkName->check(name, scope); }

const Name *Semantic::check(NestedNameSpecifierListAST *name, Scope *scope)
{ return d->checkName->check(name, scope); }

const Name *Semantic::check(ObjCSelectorAST *args, Scope *scope)
{ return d->checkName->check(args, scope); }

bool Semantic::skipFunctionBodies() const
{ return d->skipFunctionBodies; }

void Semantic::setSkipFunctionBodies(bool skipFunctionBodies)
{ d->skipFunctionBodies = skipFunctionBodies; }

int Semantic::currentVisibility() const
{ return d->visibility; }

int Semantic::switchVisibility(int visibility)
{
    int previousVisibility = d->visibility;
    d->visibility = visibility;
    return previousVisibility;
}

int Semantic::currentObjCVisibility() const
{ return d->ojbcVisibility; }

int Semantic::switchObjCVisibility(int visibility)
{
    int previousOjbCVisibility = d->ojbcVisibility;
    d->ojbcVisibility = visibility;
    return previousOjbCVisibility;
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
    case T_Q_SIGNALS:
        return Symbol::Protected;
    default:
        return Symbol::Public;
    }
}

int Semantic::visibilityForObjCAccessSpecifier(int tokenKind) const
{
    switch (tokenKind) {
    case T_AT_PUBLIC:
        return Symbol::Public;
    case T_AT_PROTECTED:
        return Symbol::Protected;
    case T_AT_PRIVATE:
        return Symbol::Private;
    case T_AT_PACKAGE:
        return Symbol::Package;
    default:
        return Symbol::Protected;
    }
}

bool Semantic::isObjCClassMethod(int tokenKind) const
{
    switch (tokenKind) {
    case T_PLUS:
        return true;
    case T_MINUS:
    default:
        return false;
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


