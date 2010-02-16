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

#ifndef CPLUSPLUS_CHECKDECLARATION_H
#define CPLUSPLUS_CHECKDECLARATION_H

#include "CPlusPlusForwardDeclarations.h"
#include "SemanticCheck.h"


namespace CPlusPlus {

class CPLUSPLUS_EXPORT CheckDeclaration: public SemanticCheck
{
public:
    CheckDeclaration(Semantic *semantic);
    virtual ~CheckDeclaration();

    void check(DeclarationAST *declaration, Scope *scope, TemplateParameters *templateParameters);

protected:
    DeclarationAST *switchDeclaration(DeclarationAST *declaration);
    Scope *switchScope(Scope *scope);
    TemplateParameters *switchTemplateParameters(TemplateParameters *templateParameters);

    void checkFunctionArguments(Function *fun);

    using ASTVisitor::visit;

    unsigned locationOfDeclaratorId(DeclaratorAST *declarator) const;

    virtual bool visit(SimpleDeclarationAST *ast);
    virtual bool visit(EmptyDeclarationAST *ast);
    virtual bool visit(AccessDeclarationAST *ast);
    virtual bool visit(QtEnumDeclarationAST *ast);
    virtual bool visit(QtFlagsDeclarationAST *ast);
    virtual bool visit(AsmDefinitionAST *ast);
    virtual bool visit(ExceptionDeclarationAST *ast);
    virtual bool visit(FunctionDefinitionAST *ast);
    virtual bool visit(LinkageBodyAST *ast);
    virtual bool visit(LinkageSpecificationAST *ast);
    virtual bool visit(NamespaceAST *ast);
    virtual bool visit(NamespaceAliasDefinitionAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);
    virtual bool visit(TemplateDeclarationAST *ast);
    virtual bool visit(TypenameTypeParameterAST *ast);
    virtual bool visit(TemplateTypeParameterAST *ast);
    virtual bool visit(UsingAST *ast);
    virtual bool visit(UsingDirectiveAST *ast);
    virtual bool visit(MemInitializerAST *ast);

    virtual bool visit(ObjCProtocolDeclarationAST *ast);
    virtual bool visit(ObjCProtocolForwardDeclarationAST *ast);
    virtual bool visit(ObjCClassDeclarationAST *ast);
    virtual bool visit(ObjCClassForwardDeclarationAST *ast);
    virtual bool visit(ObjCMethodDeclarationAST *ast);
    virtual bool visit(ObjCVisibilityDeclarationAST *ast);
    virtual bool visit(ObjCPropertyDeclarationAST *ast);

private:
    bool checkPropertyAttribute(ObjCPropertyAttributeAST *attrAst,
                                int &flags,
                                int attr);
private:
    DeclarationAST *_declaration;
    Scope *_scope;
    TemplateParameters *_templateParameters;
    bool _checkAnonymousArguments: 1;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_CHECKDECLARATION_H
