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

#include "CheckSpecifier.h"
#include "Semantic.h"
#include "AST.h"
#include "Token.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include "Names.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Control.h"
#include "Scope.h"

using namespace CPlusPlus;

CheckSpecifier::CheckSpecifier(Semantic *semantic)
    : SemanticCheck(semantic),
      _specifier(0),
      _scope(0)
{ }

CheckSpecifier::~CheckSpecifier()
{ }

FullySpecifiedType CheckSpecifier::check(SpecifierListAST *specifier, Scope *scope)
{
    FullySpecifiedType previousType = switchFullySpecifiedType(FullySpecifiedType());
    Scope *previousScope = switchScope(scope);
    SpecifierListAST *previousSpecifier = switchSpecifier(specifier);
    accept(specifier);
    (void) switchSpecifier(previousSpecifier);
    (void) switchScope(previousScope);
    return switchFullySpecifiedType(previousType);
}

FullySpecifiedType CheckSpecifier::check(ObjCTypeNameAST *typeName, Scope *scope)
{
    FullySpecifiedType previousType = switchFullySpecifiedType(FullySpecifiedType());
    Scope *previousScope = switchScope(scope);

    accept(typeName);

    (void) switchScope(previousScope);
    return switchFullySpecifiedType(previousType);
}

SpecifierListAST *CheckSpecifier::switchSpecifier(SpecifierListAST *specifier)
{
    SpecifierListAST *previousSpecifier = _specifier;
    _specifier = specifier;
    return previousSpecifier;
}

FullySpecifiedType CheckSpecifier::switchFullySpecifiedType(const FullySpecifiedType &type)
{
    FullySpecifiedType previousType = _fullySpecifiedType;
    _fullySpecifiedType = type;
    return previousType;
}

Scope *CheckSpecifier::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool CheckSpecifier::visit(SimpleSpecifierAST *ast)
{
    switch (tokenKind(ast->specifier_token)) {
        case T_CONST:
            if (_fullySpecifiedType.isConst())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setConst(true);
            break;

        case T_VOLATILE:
            if (_fullySpecifiedType.isVolatile())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setVolatile(true);
            break;

        case T_FRIEND:
            if (_fullySpecifiedType.isFriend())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setFriend(true);
            break;

        case T_REGISTER:
            if (_fullySpecifiedType.isRegister())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setRegister(true);
            break;

        case T_STATIC:
            if (_fullySpecifiedType.isStatic())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setStatic(true);
            break;

        case T_EXTERN:
            if (_fullySpecifiedType.isExtern())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setExtern(true);
            break;

        case T_MUTABLE:
            if (_fullySpecifiedType.isMutable())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setMutable(true);
            break;

        case T_TYPEDEF:
            if (_fullySpecifiedType.isTypedef())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setTypedef(true);
            break;

        case T_INLINE:
            if (_fullySpecifiedType.isInline())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setInline(true);
            break;

        case T_VIRTUAL:
            if (_fullySpecifiedType.isVirtual())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setVirtual(true);
            break;

        case T_EXPLICIT:
            if (_fullySpecifiedType.isExplicit())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setExplicit(true);
            break;

        case T_SIGNED:
            if (_fullySpecifiedType.isSigned())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setSigned(true);
            break;

        case T_UNSIGNED:
            if (_fullySpecifiedType.isUnsigned())
                translationUnit()->error(ast->specifier_token,
                                         "duplicate `%s'", spell(ast->specifier_token));
            _fullySpecifiedType.setUnsigned(true);
            break;

        case T_CHAR:
            if (_fullySpecifiedType)
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            _fullySpecifiedType.setType(control()->integerType(IntegerType::Char));
            break;

        case T_WCHAR_T:
            if (_fullySpecifiedType)
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            _fullySpecifiedType.setType(control()->integerType(IntegerType::WideChar));
            break;

        case T_BOOL:
            if (_fullySpecifiedType)
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            _fullySpecifiedType.setType(control()->integerType(IntegerType::Bool));
            break;

        case T_SHORT:
            if (_fullySpecifiedType) {
                IntegerType *intType = control()->integerType(IntegerType::Int);
                if (_fullySpecifiedType.type() != intType)
                    translationUnit()->error(ast->specifier_token,
                                             "duplicate data type in declaration");
            }
            _fullySpecifiedType.setType(control()->integerType(IntegerType::Short));
            break;

        case T_INT:
            if (_fullySpecifiedType) {
                Type *tp = _fullySpecifiedType.type();
                IntegerType *shortType = control()->integerType(IntegerType::Short);
                IntegerType *longType = control()->integerType(IntegerType::Long);
                IntegerType *longLongType = control()->integerType(IntegerType::LongLong);
                if (tp == shortType || tp == longType || tp == longLongType)
                    break;
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            }
            _fullySpecifiedType.setType(control()->integerType(IntegerType::Int));
            break;

        case T_LONG:
            if (_fullySpecifiedType) {
                Type *tp = _fullySpecifiedType.type();
                IntegerType *intType = control()->integerType(IntegerType::Int);
                IntegerType *longType = control()->integerType(IntegerType::Long);
                FloatType *doubleType = control()->floatType(FloatType::Double);
                if (tp == longType) {
                    _fullySpecifiedType.setType(control()->integerType(IntegerType::LongLong));
                    break;
                } else if (tp == doubleType) {
                    _fullySpecifiedType.setType(control()->floatType(FloatType::LongDouble));
                    break;
                } else if (tp != intType) {
                    translationUnit()->error(ast->specifier_token,
                                             "duplicate data type in declaration");
                }
            }
            _fullySpecifiedType.setType(control()->integerType(IntegerType::Long));
            break;

        case T_FLOAT:
            if (_fullySpecifiedType)
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            _fullySpecifiedType.setType(control()->floatType(FloatType::Float));
            break;

        case T_DOUBLE:
            if (_fullySpecifiedType) {
                IntegerType *longType = control()->integerType(IntegerType::Long);
                if (_fullySpecifiedType.type() == longType) {
                    _fullySpecifiedType.setType(control()->floatType(FloatType::LongDouble));
                    break;
                }
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            }
            _fullySpecifiedType.setType(control()->floatType(FloatType::Double));
            break;

        case T_VOID:
            if (_fullySpecifiedType)
                translationUnit()->error(ast->specifier_token,
                                         "duplicate data type in declaration");
            _fullySpecifiedType.setType(control()->voidType());
            break;

        default:
            break;
    } // switch

    return false;
}

bool CheckSpecifier::visit(ClassSpecifierAST *ast)
{
    unsigned sourceLocation = ast->firstToken();

    if (ast->name)
        sourceLocation = ast->name->firstToken();

    const Name *className = semantic()->check(ast->name, _scope);
    Class *klass = control()->newClass(sourceLocation, className);
    klass->setStartOffset(tokenAt(ast->firstToken()).offset);
    klass->setEndOffset(tokenAt(ast->lastToken()).offset);
    ast->symbol = klass;
    unsigned classKey = tokenKind(ast->classkey_token);
    if (classKey == T_CLASS)
        klass->setClassKey(Class::ClassKey);
    else if (classKey == T_STRUCT)
        klass->setClassKey(Class::StructKey);
    else if (classKey == T_UNION)
        klass->setClassKey(Class::UnionKey);
    klass->setVisibility(semantic()->currentVisibility());
    _scope->enterSymbol(klass);
    _fullySpecifiedType.setType(klass);

    for (BaseSpecifierListAST *it = ast->base_clause_list; it; it = it->next) {
        BaseSpecifierAST *base = it->value;
        const Name *baseClassName = semantic()->check(base->name, _scope);
        BaseClass *baseClass = control()->newBaseClass(ast->firstToken(), baseClassName);
        base->symbol = baseClass;
        if (base->virtual_token)
            baseClass->setVirtual(true);
        if (base->access_specifier_token) {
            int accessSpecifier = tokenKind(base->access_specifier_token);
            int visibility = semantic()->visibilityForAccessSpecifier(accessSpecifier);
            baseClass->setVisibility(visibility);
        }
        klass->addBaseClass(baseClass);
    }

    int visibility = semantic()->visibilityForClassKey(classKey);
    int previousVisibility = semantic()->switchVisibility(visibility);
    int previousMethodKey = semantic()->switchMethodKey(Function::NormalMethod);

    DeclarationAST *previousDeclaration = 0;
    for (DeclarationListAST *it = ast->member_specifier_list; it; it = it->next) {
        DeclarationAST *declaration = it->value;
        semantic()->check(declaration, klass->members());

        if (previousDeclaration && declaration &&
                declaration->asEmptyDeclaration() != 0 &&
                previousDeclaration->asFunctionDefinition() != 0)
            translationUnit()->warning(declaration->firstToken(), "unnecessary semicolon after function body");

        previousDeclaration = declaration;
    }

    (void) semantic()->switchMethodKey(previousMethodKey);
    (void) semantic()->switchVisibility(previousVisibility);

    return false;
}

bool CheckSpecifier::visit(NamedTypeSpecifierAST *ast)
{
    const Name *name = semantic()->check(ast->name, _scope);
    _fullySpecifiedType.setType(control()->namedType(name));
    return false;
}

bool CheckSpecifier::visit(ElaboratedTypeSpecifierAST *ast)
{
    const Name *name = semantic()->check(ast->name, _scope);
    _fullySpecifiedType.setType(control()->namedType(name));
    return false;
}

bool CheckSpecifier::visit(EnumSpecifierAST *ast)
{
    unsigned sourceLocation = ast->firstToken();
    if (ast->name)
        sourceLocation = ast->name->firstToken();

    const Name *name = semantic()->check(ast->name, _scope);
    Enum *e = control()->newEnum(sourceLocation, name);
    e->setStartOffset(tokenAt(ast->firstToken()).offset);
    e->setEndOffset(tokenAt(ast->lastToken()).offset);
    e->setVisibility(semantic()->currentVisibility());
    _scope->enterSymbol(e);
    _fullySpecifiedType.setType(e);
    for (EnumeratorListAST *it = ast->enumerator_list; it; it = it->next) {
        EnumeratorAST *enumerator = it->value;
        const Identifier *id = identifier(enumerator->identifier_token);
        if (! id)
            continue;
        const NameId *enumeratorName = control()->nameId(id);
        Declaration *decl = control()->newDeclaration(enumerator->firstToken(),
                                                      enumeratorName);

        FullySpecifiedType initTy = semantic()->check(enumerator->expression, _scope);
        e->addMember(decl);
    }
    return false;
}

bool CheckSpecifier::visit(TypeofSpecifierAST *ast)
{
    semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckSpecifier::visit(AttributeSpecifierAST * /*ast*/)
{
    return false;
}
