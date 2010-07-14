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
#include <ASTVisitor.h>
#include <QtCore/QSet>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>

namespace CPlusPlus {

class CheckSymbols:
        protected ASTVisitor,
        public QtConcurrent::RunFunctionTaskBase<CppEditor::Internal::SemanticInfo::Use>
{
public:
    virtual ~CheckSymbols();

    typedef CppEditor::Internal::SemanticInfo::Use Use;

    virtual void run();
    void runFunctor();

    typedef QFuture<Use> Future;
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

    void checkName(NameAST *ast);
    void checkNamespace(NameAST *name);
    void addTypeUsage(ClassOrNamespace *b, NameAST *ast);
    void addTypeUsage(const QList<Symbol *> &candidates, NameAST *ast);
    void addTypeUsage(const Use &use);

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

    unsigned startOfTemplateDeclaration(TemplateDeclarationAST *ast) const;
    Scope *findScope(AST *ast) const;

    void flush();

private:
    Document::Ptr _doc;
    LookupContext _context;
    QString _fileName;
    QList<Document::DiagnosticMessage> _diagnosticMessages;
    QSet<QByteArray> _potentialTypes;
    QList<ScopedSymbol *> _scopes;
    QList<TemplateDeclarationAST *> _templateDeclarationStack;
    QVector<Use> _typeUsages;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_CHECKSYMBOLS_H
