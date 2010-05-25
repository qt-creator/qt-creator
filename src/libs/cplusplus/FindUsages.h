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
#include "Semantic.h"
#include "TypeOfExpression.h"
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

    void operator()(Symbol *symbol);

    QList<Usage> usages() const;
    QList<int> references() const;

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    QString matchingLine(const Token &tk) const;
    Scope *scopeAt(unsigned tokenIndex) const;

    void reportResult(unsigned tokenIndex, const QList<Symbol *> &candidates);
    void reportResult(unsigned tokenIndex);

    bool checkCandidates(const QList<Symbol *> &candidates) const;
    void checkExpression(unsigned startToken, unsigned endToken);

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
    virtual bool visit(QtPropertyDeclarationAST *);
    virtual void endVisit(QtPropertyDeclarationAST *);

private:
    const Identifier *_id;
    Symbol *_declSymbol;
    Document::Ptr _doc;
    Snapshot _snapshot;
    LookupContext _context;
    QByteArray _source;
    Document::Ptr _exprDoc;
    Semantic _sem;
    QList<PostfixExpressionAST *> _postfixExpressionStack;
    QList<QualifiedNameAST *> _qualifiedNameStack;
    QList<int> _references;
    QList<Usage> _usages;
    int _inSimpleDeclaration;
    bool _inQProperty;
    QSet<unsigned> _processed;
    TypeOfExpression typeofExpression;
};

} // end of namespace CPlusPlus

#endif // FINDUSAGES_H
