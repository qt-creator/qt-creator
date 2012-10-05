/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPLUSPLUS_CHECKSYMBOLS_H
#define CPLUSPLUS_CHECKSYMBOLS_H

#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <texteditor/semantichighlighter.h>

#include <ASTVisitor.h>
#include <QSet>
#include <QFuture>
#include <QtConcurrentRun>

namespace CPlusPlus {

class CheckSymbols:
        protected ASTVisitor,
        public QRunnable,
        public QFutureInterface<TextEditor::SemanticHighlighter::Result>
{
public:
    virtual ~CheckSymbols();

    typedef TextEditor::SemanticHighlighter::Result Use;
    typedef CppTools::SemanticInfo::UseKind UseKind;

    virtual void run();

    typedef QFuture<Use> Future;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
        return future;
    }

    static Future go(Document::Ptr doc, const LookupContext &context, const QList<Use> &macroUses);

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

    CheckSymbols(Document::Ptr doc, const LookupContext &context, const QList<Use> &macroUses);

    bool hasVirtualDestructor(Class *klass) const;
    bool hasVirtualDestructor(ClassOrNamespace *binding) const;

    bool warning(unsigned line, unsigned column, const QString &text, unsigned length = 0);
    bool warning(AST *ast, const QString &text);

    QByteArray textOf(AST *ast) const;

    bool maybeType(const Name *name) const;
    bool maybeField(const Name *name) const;
    bool maybeStatic(const Name *name) const;
    bool maybeFunction(const Name *name) const;

    void checkNamespace(NameAST *name);
    void checkName(NameAST *ast, Scope *scope = 0);
    ClassOrNamespace *checkNestedName(QualifiedNameAST *ast);

    void addUse(const Use &use);
    void addUse(unsigned tokenIndex, UseKind kind);
    void addUse(NameAST *name, UseKind kind);

    void addType(ClassOrNamespace *b, NameAST *ast);

    bool maybeAddTypeOrStatic(const QList<LookupItem> &candidates, NameAST *ast);
    bool maybeAddField(const QList<LookupItem> &candidates, NameAST *ast);
    bool maybeAddFunction(const QList<LookupItem> &candidates, NameAST *ast, unsigned argumentCount);

    bool isTemplateClass(Symbol *s) const;

    Scope *enclosingScope() const;
    FunctionDefinitionAST *enclosingFunctionDefinition(bool skipTopOfStack = false) const;
    TemplateDeclarationAST *enclosingTemplateDeclaration() const;

    virtual bool preVisit(AST *);
    virtual void postVisit(AST *);

    virtual bool visit(NamespaceAST *);
    virtual bool visit(UsingDirectiveAST *);
    virtual bool visit(SimpleDeclarationAST *);
    virtual bool visit(TypenameTypeParameterAST *ast);
    virtual bool visit(TemplateTypeParameterAST *ast);
    virtual bool visit(FunctionDefinitionAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);

    virtual bool visit(ElaboratedTypeSpecifierAST *ast);

    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(TemplateIdAST *ast);

    virtual bool visit(MemberAccessAST *ast);
    virtual bool visit(CallAST *ast);
    virtual bool visit(NewExpressionAST *ast);

    virtual bool visit(GotoStatementAST *ast);
    virtual bool visit(LabeledStatementAST *ast);
    virtual bool visit(SimpleSpecifierAST *ast);
    virtual bool visit(ClassSpecifierAST *ast);

    virtual bool visit(MemInitializerAST *ast);
    virtual bool visit(EnumeratorAST *ast);

    NameAST *declaratorId(DeclaratorAST *ast) const;

    static unsigned referenceToken(NameAST *name);

    void flush();

private:
    Document::Ptr _doc;
    LookupContext _context;
    TypeOfExpression typeOfExpression;
    QString _fileName;
    QSet<QByteArray> _potentialTypes;
    QSet<QByteArray> _potentialFields;
    QSet<QByteArray> _potentialFunctions;
    QSet<QByteArray> _potentialStatics;
    QList<AST *> _astStack;
    QVector<Use> _usages;
    unsigned _lineOfLastUsage;
    QList<Use> _macroUses;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_CHECKSYMBOLS_H
