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

#ifndef CPLUSPLUS_PARSER_H
#define CPLUSPLUS_PARSER_H

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "Token.h"
#include "TranslationUnit.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT Parser
{
public:
    Parser(TranslationUnit *translationUnit);
    ~Parser();

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool onoff);

    bool objCEnabled() const;
    void setObjCEnabled(bool onoff);

    bool parseTranslationUnit(TranslationUnitAST *&node);

public:
    bool parseAccessSpecifier(SpecifierAST *&node);
    bool parseExpressionList(ExpressionListAST *&node);
    bool parseAbstractCoreDeclarator(DeclaratorAST *&node);
    bool parseAbstractDeclarator(DeclaratorAST *&node);
    bool parseEmptyDeclaration(DeclarationAST *&node);
    bool parseAccessDeclaration(DeclarationAST *&node);
    bool parseAdditiveExpression(ExpressionAST *&node);
    bool parseAndExpression(ExpressionAST *&node);
    bool parseAsmDefinition(DeclarationAST *&node);
    bool parseAssignmentExpression(ExpressionAST *&node);
    bool parseBaseClause(BaseSpecifierAST *&node);
    bool parseBaseSpecifier(BaseSpecifierAST *&node);
    bool parseBlockDeclaration(DeclarationAST *&node);
    bool parseCppCastExpression(ExpressionAST *&node);
    bool parseCastExpression(ExpressionAST *&node);
    bool parseClassSpecifier(SpecifierAST *&node);
    bool parseCommaExpression(ExpressionAST *&node);
    bool parseCompoundStatement(StatementAST *&node);
    bool parseBreakStatement(StatementAST *&node);
    bool parseContinueStatement(StatementAST *&node);
    bool parseGotoStatement(StatementAST *&node);
    bool parseReturnStatement(StatementAST *&node);
    bool parseCondition(ExpressionAST *&node);
    bool parseConditionalExpression(ExpressionAST *&node);
    bool parseConstantExpression(ExpressionAST *&node);
    bool parseCtorInitializer(CtorInitializerAST *&node);
    bool parseCvQualifiers(SpecifierAST *&node);
    bool parseDeclaratorOrAbstractDeclarator(DeclaratorAST *&node);
    bool parseDeclaration(DeclarationAST *&node);
    bool parseSimpleDeclaration(DeclarationAST *&node, bool acceptStructDeclarator = false);
    bool parseDeclarationStatement(StatementAST *&node);
    bool parseCoreDeclarator(DeclaratorAST *&node);
    bool parseDeclarator(DeclaratorAST *&node);
    bool parseDeleteExpression(ExpressionAST *&node);
    bool parseDoStatement(StatementAST *&node);
    bool parseElaboratedTypeSpecifier(SpecifierAST *&node);
    bool parseEnumSpecifier(SpecifierAST *&node);
    bool parseEnumerator(EnumeratorAST *&node);
    bool parseEqualityExpression(ExpressionAST *&node);
    bool parseExceptionDeclaration(ExceptionDeclarationAST *&node);
    bool parseExceptionSpecification(ExceptionSpecificationAST *&node);
    bool parseExclusiveOrExpression(ExpressionAST *&node);
    bool parseExpression(ExpressionAST *&node);
    bool parseExpressionOrDeclarationStatement(StatementAST *&node);
    bool parseExpressionStatement(StatementAST *&node);
    bool parseForInitStatement(StatementAST *&node);
    bool parseForStatement(StatementAST *&node);
    bool parseFunctionBody(StatementAST *&node);
    bool parseIfStatement(StatementAST *&node);
    bool parseInclusiveOrExpression(ExpressionAST *&node);
    bool parseInitDeclarator(DeclaratorAST *&node, bool acceptStructDeclarator);
    bool parseInitializerList(ExpressionListAST *&node);
    bool parseInitializer(ExpressionAST *&node);
    bool parseInitializerClause(ExpressionAST *&node);
    bool parseLabeledStatement(StatementAST *&node);
    bool parseLinkageBody(DeclarationAST *&node);
    bool parseLinkageSpecification(DeclarationAST *&node);
    bool parseLogicalAndExpression(ExpressionAST *&node);
    bool parseLogicalOrExpression(ExpressionAST *&node);
    bool parseMemInitializer(MemInitializerAST *&node);
    bool parseMemInitializerList(MemInitializerAST *&node);
    bool parseMemberSpecification(DeclarationAST *&node);
    bool parseMultiplicativeExpression(ExpressionAST *&node);
    bool parseTemplateId(NameAST *&node);
    bool parseClassOrNamespaceName(NameAST *&node);
    bool parseNameId(NameAST *&node);
    bool parseName(NameAST *&node, bool acceptTemplateId = true);
    bool parseNestedNameSpecifier(NestedNameSpecifierAST *&node, bool acceptTemplateId);
    bool parseNestedNameSpecifierOpt(NestedNameSpecifierAST *&name, bool acceptTemplateId);
    bool parseNamespace(DeclarationAST *&node);
    bool parseNamespaceAliasDefinition(DeclarationAST *&node);
    bool parseNewDeclarator(NewDeclaratorAST *&node);
    bool parseNewExpression(ExpressionAST *&node);
    bool parseNewInitializer(NewInitializerAST *&node);
    bool parseNewTypeId(NewTypeIdAST *&node);
    bool parseOperator(OperatorAST *&node);
    bool parseConversionFunctionId(NameAST *&node);
    bool parseOperatorFunctionId(NameAST *&node);
    bool parseParameterDeclaration(DeclarationAST *&node);
    bool parseParameterDeclarationClause(ParameterDeclarationClauseAST *&node);
    bool parseParameterDeclarationList(DeclarationAST *&node);
    bool parsePmExpression(ExpressionAST *&node);
    bool parseTypeidExpression(ExpressionAST *&node);
    bool parseTypenameCallExpression(ExpressionAST *&node);
    bool parseCorePostfixExpression(ExpressionAST *&node);
    bool parsePostfixExpression(ExpressionAST *&node);
    bool parsePostfixExpressionInternal(ExpressionAST *&node);
    bool parsePrimaryExpression(ExpressionAST *&node);
    bool parseNestedExpression(ExpressionAST *&node);
    bool parsePtrOperator(PtrOperatorAST *&node);
    bool parseRelationalExpression(ExpressionAST *&node);
    bool parseShiftExpression(ExpressionAST *&node);
    bool parseStatement(StatementAST *&node);
    bool parseThisExpression(ExpressionAST *&node);
    bool parseBoolLiteral(ExpressionAST *&node);
    bool parseNumericLiteral(ExpressionAST *&node);
    bool parseStringLiteral(ExpressionAST *&node);
    bool parseSwitchStatement(StatementAST *&node);
    bool parseTemplateArgument(ExpressionAST *&node);
    bool parseTemplateArgumentList(TemplateArgumentListAST *&node);
    bool parseTemplateDeclaration(DeclarationAST *&node);
    bool parseTemplateParameter(DeclarationAST *&node);
    bool parseTemplateParameterList(DeclarationAST *&node);
    bool parseThrowExpression(ExpressionAST *&node);
    bool parseTryBlockStatement(StatementAST *&node);
    bool parseCatchClause(CatchClauseAST *&node);
    bool parseTypeId(ExpressionAST *&node);
    bool parseTypeIdList(ExpressionListAST *&node);
    bool parseTypenameTypeParameter(DeclarationAST *&node);
    bool parseTemplateTypeParameter(DeclarationAST *&node);
    bool parseTypeParameter(DeclarationAST *&node);

    bool parseBuiltinTypeSpecifier(SpecifierAST *&node);
    bool parseAttributeSpecifier(SpecifierAST *&node);
    bool parseAttributeList(AttributeAST *&node);

    bool parseSimpleTypeSpecifier(SpecifierAST *&node)
    { return parseDeclSpecifierSeq(node, true, true); }

    bool parseTypeSpecifier(SpecifierAST *&node)
    { return parseDeclSpecifierSeq(node, true); }

    bool parseDeclSpecifierSeq(SpecifierAST *&node,
                               bool onlyTypeSpecifiers = false,
                               bool simplified = false);
    bool parseUnaryExpression(ExpressionAST *&node);
    bool parseUnqualifiedName(NameAST *&node, bool acceptTemplateId = true);
    bool parseUsing(DeclarationAST *&node);
    bool parseUsingDirective(DeclarationAST *&node);
    bool parseWhileStatement(StatementAST *&node);

    // Qt MOC run
    bool parseQtMethod(ExpressionAST *&node);

    // ObjC++
    bool parseObjCClassDeclaration(DeclarationAST *&node);
    bool parseObjCInterface(DeclarationAST *&node,
                            SpecifierAST *attributes = 0);
    bool parseObjCProtocol(DeclarationAST *&node,
                           SpecifierAST *attributes = 0);

    bool parseObjCProtocolRefs();
    bool parseObjClassInstanceVariables();
    bool parseObjCInterfaceMemberDeclaration();
    bool parseObjCInstanceVariableDeclaration(DeclarationAST *&node);
    bool parseObjCPropertyDeclaration(DeclarationAST *&node,
                                      SpecifierAST *attributes = 0);
    bool parseObjCImplementation(DeclarationAST *&node);
    bool parseObjCMethodPrototype();
    bool parseObjCPropertyAttribute();
    bool parseObjCTypeName();
    bool parseObjCSelector();
    bool parseObjCKeywordDeclaration();
    bool parseObjCTypeQualifiers();
    bool parseObjCEnd(DeclarationAST *&node);

    bool lookAtObjCSelector() const;

    bool skipUntil(int token);
    bool skipUntilDeclaration();
    bool skipUntilStatement();
    bool skip(int l, int r);

    bool lookAtCVQualifier() const;
    bool lookAtFunctionSpecifier() const;
    bool lookAtStorageClassSpecifier() const;
    bool lookAtBuiltinTypeSpecifier() const;
    bool lookAtClassKey() const;
    bool lookAtAssignmentOperator() const;

    void match(int kind, unsigned *token);

    bool maybeFunctionCall(SimpleDeclarationAST *simpleDecl) const;
    bool maybeSimpleExpression(SimpleDeclarationAST *simpleDecl) const;

private:
    bool switchTemplateArguments(bool templateArguments);
    bool blockErrors(bool block);

    inline const Token &tok(int i = 1) const
    { return _translationUnit->tokenAt(_tokenIndex + i - 1); }

    inline int LA(int n = 1) const
    { return _translationUnit->tokenKind(_tokenIndex + n - 1); }

    inline int consumeToken()
    { return _tokenIndex++; }

    inline unsigned cursor() const
    { return _tokenIndex; }

    inline void rewind(unsigned cursor)
    { _tokenIndex = cursor; }

private:
    TranslationUnit *_translationUnit;
    Control *_control;
    MemoryPool *_pool;
    unsigned _tokenIndex;
    bool _templateArguments: 1;
    bool _qtMocRunEnabled: 1;
    bool _objCEnabled: 1;
    bool _inFunctionBody: 1;
    bool _inObjCImplementationContext: 1;

private:
    Parser(const Parser& source);
    void operator =(const Parser& source);
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_PARSER_H
