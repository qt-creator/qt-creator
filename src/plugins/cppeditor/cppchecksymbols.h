// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppsemanticinfo.h"
#include "semantichighlighter.h"

#include <cplusplus/TypeOfExpression.h>

#include <QFuture>
#include <QSet>
#include <QThreadPool>

namespace CppEditor {

class CPPEDITOR_EXPORT CheckSymbols:
        public QObject,
        protected CPlusPlus::ASTVisitor,
        public QRunnable,
        public QFutureInterface<TextEditor::HighlightingResult>
{
    Q_OBJECT
public:
    ~CheckSymbols() override;

    using Result = TextEditor::HighlightingResult;
    using Kind = SemanticHighlighter::Kind;

    void run() override;

    using Future = QFuture<Result>;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
        return future;
    }

    static Future go(CPlusPlus::Document::Ptr doc,
                     const QString &content,
                     const CPlusPlus::LookupContext &context,
                     const QList<Result> &macroUses);
    static CheckSymbols * create(CPlusPlus::Document::Ptr doc,
                                 const QString &content,
                                 const CPlusPlus::LookupContext &context,
                                 const QList<Result> &macroUses);

    static QMap<int, QList<Result> > chunks(const QFuture<Result> &future, int from, int to)
    {
        QMap<int, QList<Result> > chunks;

        for (int i = from; i < to; ++i) {
            const Result use = future.resultAt(i);
            if (use.isInvalid())
                continue;

            const int blockNumber = use.line - 1;
            chunks[blockNumber].append(use);
        }

        return chunks;
    }

signals:
    void codeWarningsUpdated(CPlusPlus::Document::Ptr document,
                             const QList<CPlusPlus::Document::DiagnosticMessage> &selections);

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    enum FunctionKind
    {
        FunctionDeclaration,
        FunctionCall
    };

    CheckSymbols(CPlusPlus::Document::Ptr doc,
                 const QString &content,
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
    void checkName(CPlusPlus::NameAST *ast, CPlusPlus::Scope *scope = nullptr);
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
                          CPlusPlus::NameAST *ast, int argumentCount,
                          FunctionKind functionKind);

    bool isTemplateClass(CPlusPlus::Symbol *s) const;

    CPlusPlus::Scope *enclosingScope() const;
    CPlusPlus::FunctionDefinitionAST *enclosingFunctionDefinition(bool skipTopOfStack = false) const;
    CPlusPlus::TemplateDeclarationAST *enclosingTemplateDeclaration() const;

    bool preVisit(CPlusPlus::AST *) override;
    void postVisit(CPlusPlus::AST *) override;

    bool visit(CPlusPlus::NamespaceAST *) override;
    bool visit(CPlusPlus::UsingDirectiveAST *) override;
    bool visit(CPlusPlus::SimpleDeclarationAST *) override;
    bool visit(CPlusPlus::TypenameTypeParameterAST *ast) override;
    bool visit(CPlusPlus::TemplateTypeParameterAST *ast) override;
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;
    bool visit(CPlusPlus::ParameterDeclarationAST *ast) override;

    bool visit(CPlusPlus::ElaboratedTypeSpecifierAST *ast) override;

    bool visit(CPlusPlus::ObjCProtocolDeclarationAST *ast) override;
    bool visit(CPlusPlus::ObjCProtocolForwardDeclarationAST *ast) override;
    bool visit(CPlusPlus::ObjCClassDeclarationAST *ast) override;
    bool visit(CPlusPlus::ObjCClassForwardDeclarationAST *ast) override;
    bool visit(CPlusPlus::ObjCProtocolRefsAST *ast) override;

    bool visit(CPlusPlus::SimpleNameAST *ast) override;
    bool visit(CPlusPlus::DestructorNameAST *ast) override;
    bool visit(CPlusPlus::QualifiedNameAST *ast) override;
    bool visit(CPlusPlus::TemplateIdAST *ast) override;

    bool visit(CPlusPlus::MemberAccessAST *ast) override;
    bool visit(CPlusPlus::CallAST *ast) override;
    bool visit(CPlusPlus::ObjCSelectorArgumentAST *ast) override;
    bool visit(CPlusPlus::NewExpressionAST *ast) override;

    bool visit(CPlusPlus::GotoStatementAST *ast) override;
    bool visit(CPlusPlus::LabeledStatementAST *ast) override;
    bool visit(CPlusPlus::SimpleSpecifierAST *ast) override;
    bool visit(CPlusPlus::ClassSpecifierAST *ast) override;

    bool visit(CPlusPlus::MemInitializerAST *ast) override;
    bool visit(CPlusPlus::EnumeratorAST *ast) override;

    bool visit(CPlusPlus::DotDesignatorAST *ast) override;

    CPlusPlus::NameAST *declaratorId(CPlusPlus::DeclaratorAST *ast) const;

    static unsigned referenceToken(CPlusPlus::NameAST *name);

    void flush();

private:
    bool isConstructorDeclaration(CPlusPlus::Symbol *declaration);

    CPlusPlus::Document::Ptr _doc;
    const QString _content;
    CPlusPlus::LookupContext _context;
    CPlusPlus::TypeOfExpression typeOfExpression;
    Utils::FilePath _filePath;
    QSet<QByteArray> _potentialTypes;
    QSet<QByteArray> _potentialFields;
    QSet<QByteArray> _potentialFunctions;
    QSet<QByteArray> _potentialStatics;
    QList<CPlusPlus::AST *> _astStack;
    QList<Result> _usages;
    QList<CPlusPlus::Document::DiagnosticMessage> _diagMsgs;
    int _chunkSize;
    int _lineOfLastUsage;
    QList<Result> _macroUses;
};

} // namespace CppEditor
