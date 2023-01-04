// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "CppDocument.h"
#include "LookupContext.h"
#include "PreprocessorEnvironment.h"

#include <cplusplus/ASTfwd.h>

#include <QMap>
#include <QObject>
#include <QString>
#include <QByteArray>

namespace CPlusPlus {

class Environment;
class Macro;

class CPLUSPLUS_EXPORT TypeOfExpression
{
    Q_DISABLE_COPY(TypeOfExpression)

public:
    TypeOfExpression();

    /**
     * Sets the documents used to evaluate expressions. Should be set before
     * calling this functor.
     *
     * Also clears the lookup context, so can be used to make sure references
     * to the documents previously used are removed.
     */
    void init(Document::Ptr thisDocument,
              const Snapshot &snapshot,
              QSharedPointer<CreateBindings> bindings = QSharedPointer<CreateBindings>(),
              const QSet<const Declaration *> &autoDeclarationsBeingResolved
                = QSet<const Declaration *>());

    enum PreprocessMode {
        NoPreprocess,
        Preprocess
    };

    /**
     * Returns a list of possible fully specified types associated with the
     * given expression.
     *
     * NOTE: The fully specified types only stay valid for as long as this
     * expression evaluator instance still exists, and no new call to evaluate
     * has been made!
     *
     * @param utf8code          The code of expression to evaluate.
     * @param scope             The scope enclosing the expression.
     */
    QList<LookupItem> operator()(const QByteArray &utf8code,
                                 Scope *scope,
                                 PreprocessMode mode = NoPreprocess);

    /**
     * Returns a list of possible fully specified types associated with the
     * given expression AST from the given document.
     *
     * NOTE: The fully specified types only stay valid for as long as this
     * expression evaluator instance still exists, and no new call to evaluate
     * has been made!
     *
     * @param expression        The expression to evaluate.
     * @param document          The document in which the expression lives.
     * @param scope             The scope enclosing the expression.
     */
    QList<LookupItem> operator()(ExpressionAST *expression,
                                 Document::Ptr document,
                                 Scope *scope);

    QList<LookupItem> reference(const QByteArray &utf8code,
                                Scope *scope,
                                PreprocessMode mode = NoPreprocess);

    QList<LookupItem> reference(ExpressionAST *expression,
                                Document::Ptr document,
                                Scope *scope);

    // Returns UTF-8.
    QByteArray preprocess(const QByteArray &utf8code) const;

    /**
     * Returns the AST of the last evaluated expression.
     */
    ExpressionAST *ast() const;

    /**
     * Returns the lookup context of the last evaluated expression.
     */
    const LookupContext &context() const;
    Scope *scope() const;

    ExpressionAST *expressionAST() const;
    QByteArray preprocessedExpression(const QByteArray &utf8code) const;

    void setExpandTemplates(bool expandTemplates)
    {
        if (m_bindings)
            m_bindings->setExpandTemplates(expandTemplates);
        m_expandTemplates = expandTemplates;
    }

private:

    void processEnvironment(Document::Ptr doc, Environment *env,
                            QSet<QString> *processed) const;


private:
    Document::Ptr m_thisDocument;
    Snapshot m_snapshot;
    QSharedPointer<CreateBindings> m_bindings;
    ExpressionAST *m_ast;
    Scope *m_scope;
    LookupContext m_lookupContext;
    mutable QSharedPointer<Environment> m_environment;

    bool m_expandTemplates;

    // FIXME: This is a temporary hack to avoid dangling pointers.
    // Keep the expression documents and thus all the symbols and
    // their types alive until they are not needed any more.
    QList<Document::Ptr> m_documents;

    QSet<const Declaration *> m_autoDeclarationsBeingResolved;
};

ExpressionAST CPLUSPLUS_EXPORT *extractExpressionAST(Document::Ptr doc);
Document::Ptr CPLUSPLUS_EXPORT documentForExpression(const QByteArray &utf8code);

} // namespace CPlusPlus
