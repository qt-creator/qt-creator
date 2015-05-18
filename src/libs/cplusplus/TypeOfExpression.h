/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CPLUSPLUS_TYPEOFEXPRESSION_H
#define CPLUSPLUS_TYPEOFEXPRESSION_H

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
              CreateBindings::Ptr bindings = CreateBindings::Ptr(),
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
    CreateBindings::Ptr m_bindings;
    ExpressionAST *m_ast;
    Scope *m_scope;
    LookupContext m_lookupContext;
    mutable QSharedPointer<Environment> m_environment;
    QSet<const Declaration *> m_autoDeclarationsBeingResolved;
    bool m_expandTemplates;
};

ExpressionAST CPLUSPLUS_EXPORT *extractExpressionAST(Document::Ptr doc);
Document::Ptr CPLUSPLUS_EXPORT documentForExpression(const QByteArray &utf8code);

} // namespace CPlusPlus

#endif // CPLUSPLUS_TYPEOFEXPRESSION_H
