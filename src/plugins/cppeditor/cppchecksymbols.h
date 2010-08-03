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

#ifndef CPLUSPLUS_CHECKSYMBOLS_H
#define CPLUSPLUS_CHECKSYMBOLS_H

#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <ASTVisitor.h>
#include <QtCore/QSet>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>

namespace CPlusPlus {

class CheckSymbols:
        protected ASTVisitor,
        public QRunnable,
        public QFutureInterface<CppEditor::Internal::SemanticInfo::Use>
{
public:
    virtual ~CheckSymbols();

    typedef CppEditor::Internal::SemanticInfo::Use Use;

    virtual void run();

    typedef QFuture<Use> Future;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::IdlePriority);
        return future;
    }

    static Future go(Document::Ptr doc, const LookupContext &context);

    static QMap<int, QVector<Use> > chunks(const QFuture<Use> &future, int from, int to)
    {
        QMap<int, QVector<Use> > chunks;

        for (int i = from; i < to; ++i) {
            const Use use = future.resultAt(i);
            if (! use.line)
                continue; // skip it, it's an invalid use.

            const int blockNumber = use.line - 1;
            chunks[blockNumber].append(use);
        }

        return chunks;
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    CheckSymbols(Document::Ptr doc, const LookupContext &context);

    bool warning(unsigned line, unsigned column, const QString &text, unsigned length = 0);
    bool warning(AST *ast, const QString &text);

    void checkName(NameAST *ast, Scope *scope = 0);
    void checkNamespace(NameAST *name);
    void addUsage(ClassOrNamespace *b, NameAST *ast);
    void addUsage(const QList<LookupItem> &candidates, NameAST *ast);
    void addUsage(const Use &use);

    void checkMemberName(NameAST *ast);
    void addMemberUsage(const QList<LookupItem> &candidates, NameAST *ast);

    virtual bool preVisit(AST *);

    virtual bool visit(NamespaceAST *);
    virtual bool visit(UsingDirectiveAST *);
    virtual bool visit(SimpleDeclarationAST *);
    virtual bool visit(NamedTypeSpecifierAST *);

    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(TemplateIdAST *ast);

    virtual bool visit(TemplateDeclarationAST *ast);
    virtual void endVisit(TemplateDeclarationAST *ast);

    virtual bool visit(TypenameTypeParameterAST *ast);
    virtual bool visit(TemplateTypeParameterAST *ast);

    virtual bool visit(FunctionDefinitionAST *ast);
    virtual bool visit(MemberAccessAST *ast);

    virtual bool visit(MemInitializerAST *ast);

    unsigned startOfTemplateDeclaration(TemplateDeclarationAST *ast) const;
    Scope *findScope(AST *ast) const;

    void flush();

private:
    Document::Ptr _doc;
    LookupContext _context;
    TypeOfExpression typeOfExpression;
    QString _fileName;
    QList<Document::DiagnosticMessage> _diagnosticMessages;
    QSet<QByteArray> _potentialTypes;
    QSet<QByteArray> _potentialMembers;
    QList<ScopedSymbol *> _scopes;
    QList<TemplateDeclarationAST *> _templateDeclarationStack;
    QList<FunctionDefinitionAST *> _functionDefinitionStack;
    QVector<Use> _usages;
    bool _flushRequested;
    unsigned _flushLine;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_CHECKSYMBOLS_H
