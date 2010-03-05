/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHECKUNDEFINEDSYMBOLS_H
#define CHECKUNDEFINEDSYMBOLS_H

#include "CppDocument.h"
#include "CppBindings.h"

#include <ASTVisitor.h>
#include <QtCore/QSet>
#include <QtCore/QByteArray>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT CheckUndefinedSymbols: protected ASTVisitor
{
public:
    CheckUndefinedSymbols(Document::Ptr doc);
    virtual ~CheckUndefinedSymbols();

    void setGlobalNamespaceBinding(NamespaceBindingPtr globalNamespaceBinding);

    void operator()(AST *ast);

protected:
    using ASTVisitor::visit;

    bool isType(const Identifier *id) const;
    bool isType(const QByteArray &name) const;

    void addType(const Name *name);
    void buildTypeMap(Class *klass);
    void buildMemberTypeMap(Symbol *member);
    void buildTypeMap(NamespaceBinding *binding, QSet<NamespaceBinding *> *processed);
    void addProtocol(const Name *name);
    bool isProtocol(const QByteArray &name) const;

    FunctionDeclaratorAST *currentFunctionDeclarator() const;
    CompoundStatementAST *compoundStatement() const;
    bool qobjectCheck() const;

    QByteArray templateParameterName(NameAST *ast) const;
    QByteArray templateParameterName(DeclarationAST *ast) const;

    virtual bool visit(FunctionDeclaratorAST *ast);
    virtual void endVisit(FunctionDeclaratorAST *ast);

    virtual bool visit(TypeofSpecifierAST *ast);
    virtual bool visit(NamedTypeSpecifierAST *ast);

    virtual bool visit(TemplateDeclarationAST *ast);
    virtual void endVisit(TemplateDeclarationAST *);

    virtual bool visit(ClassSpecifierAST *ast);
    virtual void endVisit(ClassSpecifierAST *);

    virtual bool visit(FunctionDefinitionAST *ast);
    virtual void endVisit(FunctionDefinitionAST *ast);

    virtual bool visit(CompoundStatementAST *ast);
    virtual void endVisit(CompoundStatementAST *ast);

    virtual bool visit(SimpleDeclarationAST *ast);
    virtual bool visit(BaseSpecifierAST *base);
    virtual bool visit(UsingDirectiveAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(CastExpressionAST *ast);
    virtual bool visit(SizeofExpressionAST *ast);

    virtual bool visit(ObjCClassDeclarationAST *ast);
    virtual bool visit(ObjCProtocolRefsAST *ast);
    virtual bool visit(ObjCPropertyDeclarationAST *ast);

    virtual bool visit(QtEnumDeclarationAST *ast);
    virtual bool visit(QtFlagsDeclarationAST *ast);
    virtual bool visit(QtPropertyDeclarationAST *ast);

private:
    Document::Ptr _doc;
    NamespaceBindingPtr _globalNamespaceBinding;
    QList<bool> _qobjectStack;
    QList<FunctionDeclaratorAST *> _functionDeclaratorStack;
    QList<TemplateDeclarationAST *> _templateDeclarationStack;
    QList<CompoundStatementAST *> _compoundStatementStack;
    QSet<QByteArray> _types;
    QSet<QByteArray> _protocols;
    QSet<QByteArray> _namespaceNames;
};

} // end of namespace CPlusPlus

#endif // CHECKUNDEFINEDSYMBOLS_H
