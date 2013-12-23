/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPLUSPLUSCHECKSYMBOLS_H
#define CPLUSPLUSCHECKSYMBOLS_H

#include "cpptools_global.h"
#include "cpphighlightingsupport.h"
#include "cppsemanticinfo.h"

#include <cplusplus/TypeOfExpression.h>

#include <QSet>
#include <QFuture>
#include <QtConcurrentRun>

namespace CppTools {

class CPPTOOLS_EXPORT CheckSymbols:
        protected CPlusPlus::ASTVisitor,
        public QRunnable,
        public QFutureInterface<TextEditor::HighlightingResult>
{
public:
    virtual ~CheckSymbols();

    typedef TextEditor::HighlightingResult Result;
    typedef CppHighlightingSupport::Kind Kind;

    virtual void run();

    typedef QFuture<Result> Future;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
        return future;
    }

    static Future go(CPlusPlus::Document::Ptr doc,
                     const CPlusPlus::LookupContext &context,
                     const QList<Result> &macroUses);

    static QMap<int, QVector<Result> > chunks(const QFuture<Result> &future, int from, int to)
    {
        QMap<int, QVector<Result> > chunks;

        for (int i = from; i < to; ++i) {
            const Result use = future.resultAt(i);
            if (use.isInvalid())
                continue;

            const int blockNumber = use.line - 1;
            chunks[blockNumber].append(use);
        }

        return chunks;
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    CheckSymbols(CPlusPlus::Document::Ptr doc,
                 const CPlusPlus::LookupContext &context,
                 const QList<Result> &otherUses);

    bool hasVirtualDestructor(CPlusPlus::Class *klass) const;
    bool hasVirtualDestructor(CPlusPlus::ClassOrNamespace *binding) const;

    bool warning(unsigned line, unsigned column, const QString &text, unsigned length = 0);
    bool warning(CPlusPlus::AST *ast, const QString &text);

    QByteArray textOf(CPlusPlus::AST *ast) const;

    bool maybeType(const CPlusPlus::Name *name) const;
    bool maybeField(const CPlusPlus::Name *name) const;
    bool maybeStatic(const CPlusPlus::Name *name) const;
    bool maybeFunction(const CPlusPlus::Name *name) const;

    void checkNamespace(CPlusPlus::NameAST *name);
    void checkName(CPlusPlus::NameAST *ast, CPlusPlus::Scope *scope = 0);
    CPlusPlus::ClassOrNamespace *checkNestedName(CPlusPlus::QualifiedNameAST *ast);

    void addUse(const Result &use);
    void addUse(unsigned tokenIndex, Kind kind);
    void addUse(CPlusPlus::NameAST *name, Kind kind);

    void addType(CPlusPlus::ClassOrNamespace *b, CPlusPlus::NameAST *ast);

    bool maybeAddTypeOrStatic(const QList<CPlusPlus::LookupItem> &candidates,
                              CPlusPlus::NameAST *ast);
    bool maybeAddField(const QList<CPlusPlus::LookupItem> &candidates,
                       CPlusPlus::NameAST *ast);
    bool maybeAddFunction(const QList<CPlusPlus::LookupItem> &candidates,
                          CPlusPlus::NameAST *ast, unsigned argumentCount);

    bool isTemplateClass(CPlusPlus::Symbol *s) const;

    CPlusPlus::Scope *enclosingScope() const;
    CPlusPlus::FunctionDefinitionAST *enclosingFunctionDefinition(bool skipTopOfStack = false) const;
    CPlusPlus::TemplateDeclarationAST *enclosingTemplateDeclaration() const;

    virtual bool preVisit(CPlusPlus::AST *);
    virtual void postVisit(CPlusPlus::AST *);

    virtual bool visit(CPlusPlus::NamespaceAST *);
    virtual bool visit(CPlusPlus::UsingDirectiveAST *);
    virtual bool visit(CPlusPlus::SimpleDeclarationAST *);
    virtual bool visit(CPlusPlus::TypenameTypeParameterAST *ast);
    virtual bool visit(CPlusPlus::TemplateTypeParameterAST *ast);
    virtual bool visit(CPlusPlus::FunctionDefinitionAST *ast);
    virtual bool visit(CPlusPlus::ParameterDeclarationAST *ast);

    virtual bool visit(CPlusPlus::ElaboratedTypeSpecifierAST *ast);

    virtual bool visit(CPlusPlus::SimpleNameAST *ast);
    virtual bool visit(CPlusPlus::DestructorNameAST *ast);
    virtual bool visit(CPlusPlus::QualifiedNameAST *ast);
    virtual bool visit(CPlusPlus::TemplateIdAST *ast);

    virtual bool visit(CPlusPlus::MemberAccessAST *ast);
    virtual bool visit(CPlusPlus::CallAST *ast);
    virtual bool visit(CPlusPlus::NewExpressionAST *ast);

    virtual bool visit(CPlusPlus::GotoStatementAST *ast);
    virtual bool visit(CPlusPlus::LabeledStatementAST *ast);
    virtual bool visit(CPlusPlus::SimpleSpecifierAST *ast);
    virtual bool visit(CPlusPlus::ClassSpecifierAST *ast);

    virtual bool visit(CPlusPlus::MemInitializerAST *ast);
    virtual bool visit(CPlusPlus::EnumeratorAST *ast);

    CPlusPlus::NameAST *declaratorId(CPlusPlus::DeclaratorAST *ast) const;

    static unsigned referenceToken(CPlusPlus::NameAST *name);

    void flush();

private:
    bool isConstructorDeclaration(CPlusPlus::Symbol *declaration);

    CPlusPlus::Document::Ptr _doc;
    CPlusPlus::LookupContext _context;
    CPlusPlus::TypeOfExpression typeOfExpression;
    QString _fileName;
    QSet<QByteArray> _potentialTypes;
    QSet<QByteArray> _potentialFields;
    QSet<QByteArray> _potentialFunctions;
    QSet<QByteArray> _potentialStatics;
    QList<CPlusPlus::AST *> _astStack;
    QVector<Result> _usages;
    int _chunkSize;
    unsigned _lineOfLastUsage;
    QList<Result> _macroUses;
};

} // namespace CppTools

#endif // CPLUSPLUSCHECKSYMBOLS_H
