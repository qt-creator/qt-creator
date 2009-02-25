/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "TypeOfExpression.h"

#include <AST.h>
#include <TranslationUnit.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/pp.h>
#include <QSet>

using namespace CPlusPlus;

TypeOfExpression::TypeOfExpression():
    m_ast(0)
{
}

Snapshot TypeOfExpression::snapshot() const
{
    return m_snapshot;
}

void TypeOfExpression::setSnapshot(const Snapshot &documents)
{
    m_snapshot = documents;
    m_lookupContext = LookupContext();
}

QList<TypeOfExpression::Result> TypeOfExpression::operator()(const QString &expression,
                                                             Document::Ptr document,
                                                             Symbol *lastVisibleSymbol,
                                                             PreprocessMode mode)
{
    QString code = expression;
    if (mode == Preprocess)
        code = preprocessedExpression(expression, m_snapshot, document);
    Document::Ptr expressionDoc = documentForExpression(code);
    m_ast = extractExpressionAST(expressionDoc);

    m_lookupContext = LookupContext(lastVisibleSymbol, expressionDoc,
                                    document, m_snapshot);

    ResolveExpression resolveExpression(m_lookupContext);
    return resolveExpression(m_ast);
}

QString TypeOfExpression::preprocess(const QString &expression,
                                     Document::Ptr document) const
{
    return preprocessedExpression(expression, m_snapshot, document);
}

ExpressionAST *TypeOfExpression::ast() const
{
    return m_ast;
}

const LookupContext &TypeOfExpression::lookupContext() const
{
    return m_lookupContext;
}

ExpressionAST *TypeOfExpression::expressionAST() const
{
    return extractExpressionAST(m_lookupContext.expressionDocument());
}

ExpressionAST *TypeOfExpression::extractExpressionAST(Document::Ptr doc) const
{
    if (! doc->translationUnit()->ast())
        return 0;

    return doc->translationUnit()->ast()->asExpression();
}

Document::Ptr TypeOfExpression::documentForExpression(const QString &expression) const
{
    // create the expression's AST.
    Document::Ptr doc = Document::create(QLatin1String("<completion>"));
    const QByteArray bytes = expression.toUtf8();
    doc->setSource(bytes);
    doc->parse(Document::ParseExpression);
    return doc;
}

void TypeOfExpression::processEnvironment(Snapshot documents,
                                          Document::Ptr doc, Environment *env,
                                          QSet<QString> *processed) const
{
    if (! doc)
        return;
    if (processed->contains(doc->fileName()))
        return;
    processed->insert(doc->fileName());
    foreach (const Document::Include &incl, doc->includes()) {
        processEnvironment(documents,
                           documents.value(incl.fileName()),
                           env, processed);
    }
    foreach (const Macro &macro, doc->definedMacros())
        env->bind(macro);
}

QString TypeOfExpression::preprocessedExpression(const QString &expression,
                                                 Snapshot documents,
                                                 Document::Ptr thisDocument) const
{
    Environment env;
    QSet<QString> processed;
    processEnvironment(documents, thisDocument,
                       &env, &processed);
    const QByteArray code = expression.toUtf8();
    Preprocessor preproc(0, env);
    QByteArray preprocessedCode;
    preproc("<expression>", code, &preprocessedCode);
    return QString::fromUtf8(preprocessedCode);
}
