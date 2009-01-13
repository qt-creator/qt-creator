/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef CPLUSPLUS_TYPEOFEXPRESSION_H
#define CPLUSPLUS_TYPEOFEXPRESSION_H

#include <ASTfwd.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace CPlusPlus {

class Environment;
class Macro;

class CPLUSPLUS_EXPORT TypeOfExpression
{
public:
    typedef QPair<FullySpecifiedType, Symbol *> Result;

public:
    TypeOfExpression();

    Snapshot snapshot() const;

    /**
     * Sets the documents used to evaluate expressions. Should be set before
     * calling this functor.
     *
     * Also clears the lookup context, so can be used to make sure references
     * to the documents previously used are removed.
     */
    void setSnapshot(const Snapshot &documents);

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
     * @param expression        The expression to evaluate.
     * @param document          The document the expression is part of.
     * @param lastVisibleSymbol The last visible symbol in the document.
     */
    QList<Result> operator()(const QString &expression, Document::Ptr document,
                             Symbol *lastVisibleSymbol,
                             PreprocessMode mode = NoPreprocess);

    QString preprocess(const QString &expression, Document::Ptr document) const;

    /**
     * Returns the AST of the last evaluated expression.
     */
    ExpressionAST *ast() const;

    /**
     * Returns the lookup context of the last evaluated expression.
     */
    const LookupContext &lookupContext() const;

    ExpressionAST *expressionAST() const;

private:
    ExpressionAST *extractExpressionAST(Document::Ptr doc) const;
    Document::Ptr documentForExpression(const QString &expression) const;

    void processEnvironment(CPlusPlus::Snapshot documents,
                            CPlusPlus::Document::Ptr doc, CPlusPlus::Environment *env,
                            QSet<QString> *processed) const;

    QString preprocessedExpression(const QString &expression,
                                   CPlusPlus::Snapshot documents,
                                   CPlusPlus::Document::Ptr thisDocument) const;

    Snapshot m_snapshot;
    ExpressionAST *m_ast;
    LookupContext m_lookupContext;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_TYPEOFEXPRESSION_H
