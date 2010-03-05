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

#ifndef FINDUSAGES_H
#define FINDUSAGES_H

#include "LookupContext.h"
#include "CppDocument.h"
#include "CppBindings.h"
#include "Semantic.h"
#include <ASTVisitor.h>
#include <QtCore/QSet>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Usage
{
public:
    Usage()
        : line(0), col(0), len(0) {}

    Usage(const QString &path, const QString &lineText, int line, int col, int len)
        : path(path), lineText(lineText), line(line), col(col), len(len) {}

public:
    QString path;
    QString lineText;
    int line;
    int col;
    int len;
};

class CPLUSPLUS_EXPORT FindUsages: protected ASTVisitor
{
public:
    FindUsages(Document::Ptr doc, const Snapshot &snapshot);

    void setGlobalNamespaceBinding(NamespaceBindingPtr globalNamespaceBinding);

    void operator()(Symbol *symbol);

    QList<Usage> usages() const;
    QList<int> references() const;

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    QString matchingLine(const Token &tk) const;

    void reportResult(unsigned tokenIndex, const QList<Symbol *> &candidates);
    void reportResult(unsigned tokenIndex);

    bool checkSymbol(Symbol *symbol) const;
    bool checkCandidates(const QList<Symbol *> &candidates) const;
    bool checkScope(Symbol *symbol, Symbol *otherSymbol) const;
    void checkExpression(unsigned startToken, unsigned endToken);

    LookupContext currentContext(AST *ast);

    void ensureNameIsValid(NameAST *ast);

    virtual bool visit(MemInitializerAST *ast);
    virtual bool visit(PostfixExpressionAST *ast);
    virtual void endVisit(PostfixExpressionAST *);
    virtual bool visit(MemberAccessAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(EnumeratorAST *ast);
    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(TemplateIdAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);
    virtual bool visit(ExpressionOrDeclarationStatementAST *ast);
    virtual bool visit(FunctionDeclaratorAST *ast);
    virtual bool visit(SimpleDeclarationAST *);
    virtual bool visit(ObjCSelectorAST *ast);

private:
    const Identifier *_id;
    Symbol *_declSymbol;
    Document::Ptr _doc;
    Snapshot _snapshot;
    QByteArray _source;
    Document::Ptr _exprDoc;
    Semantic _sem;
    NamespaceBindingPtr _globalNamespaceBinding;
    QList<PostfixExpressionAST *> _postfixExpressionStack;
    QList<QualifiedNameAST *> _qualifiedNameStack;
    QList<int> _references;
    QList<Usage> _usages;
    LookupContext _previousContext;
    int _inSimpleDeclaration;
    QSet<unsigned> _processed;
};

} // end of namespace CPlusPlus

#endif // FINDUSAGES_H
