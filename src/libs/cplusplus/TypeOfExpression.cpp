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

#include "TypeOfExpression.h"
#include <TranslationUnit.h>
#include "LookupContext.h"
#include "ResolveExpression.h"
#include "pp.h"

#include <AST.h>
#include <Symbol.h>
#include <QSet>

using namespace CPlusPlus;

TypeOfExpression::TypeOfExpression():
    m_ast(0),
    m_scope(0),
    m_expandTemplates(false)
{
}

void TypeOfExpression::reset()
{
    m_thisDocument.clear();
    m_snapshot = Snapshot();
    m_ast = 0;
    m_scope = 0;
    m_lookupContext = LookupContext();
    m_bindings.clear();
    m_environment.clear();
}

void TypeOfExpression::init(Document::Ptr thisDocument, const Snapshot &snapshot,
                            QSharedPointer<CreateBindings> bindings)
{
    m_thisDocument = thisDocument;
    m_snapshot = snapshot;
    m_ast = 0;
    m_scope = 0;
    m_lookupContext = LookupContext();
    m_bindings = bindings;
    m_environment.clear();
}

QList<LookupItem> TypeOfExpression::operator()(const QByteArray &utf8code,
                                               Scope *scope,
                                               PreprocessMode mode)
{
    Document::Ptr expressionDoc;
    if (mode == Preprocess)
        expressionDoc = documentForExpression(preprocessedExpression(utf8code));
    else
        expressionDoc = documentForExpression(utf8code);
    expressionDoc->check();
    return this->operator ()(extractExpressionAST(expressionDoc),
                             expressionDoc,
                             scope);
}


QList<LookupItem> TypeOfExpression::reference(const QByteArray &utf8code,
                                              Scope *scope,
                                              PreprocessMode mode)
{
    Document::Ptr expressionDoc;
    if (mode == Preprocess)
        expressionDoc = documentForExpression(preprocessedExpression(utf8code));
    else
        expressionDoc = documentForExpression(utf8code);
    expressionDoc->check();
    return reference(extractExpressionAST(expressionDoc), expressionDoc, scope);
}

QList<LookupItem> TypeOfExpression::operator()(ExpressionAST *expression,
                                               Document::Ptr document,
                                               Scope *scope)
{
    m_ast = expression;

    m_scope = scope;

    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot);
    m_lookupContext.setBindings(m_bindings);
    m_lookupContext.setExpandTemplates(m_expandTemplates);

    ResolveExpression resolve(m_lookupContext);
    const QList<LookupItem> items = resolve(m_ast, scope);

    if (! m_bindings)
        m_lookupContext = resolve.context();

    return items;
}

QList<LookupItem> TypeOfExpression::reference(ExpressionAST *expression,
                                              Document::Ptr document,
                                              Scope *scope)
{
    m_ast = expression;

    m_scope = scope;

    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot);
    m_lookupContext.setBindings(m_bindings);
    m_lookupContext.setExpandTemplates(m_expandTemplates);

    ResolveExpression resolve(m_lookupContext);
    const QList<LookupItem> items = resolve.reference(m_ast, scope);

    if (! m_bindings)
        m_lookupContext = resolve.context();

    return items;
}

QByteArray TypeOfExpression::preprocess(const QByteArray &utf8code) const
{
    return preprocessedExpression(utf8code);
}

ExpressionAST *TypeOfExpression::ast() const
{
    return m_ast;
}

Scope *TypeOfExpression::scope() const
{
    return m_scope;
}

const LookupContext &TypeOfExpression::context() const
{
    return m_lookupContext;
}

ExpressionAST *TypeOfExpression::expressionAST() const
{
    return extractExpressionAST(m_lookupContext.expressionDocument());
}

void TypeOfExpression::processEnvironment(Document::Ptr doc, Environment *env,
                                          QSet<QString> *processed) const
{
    if (doc && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        foreach (const Document::Include &incl, doc->includes())
            processEnvironment(m_snapshot.document(incl.fileName()), env, processed);

        foreach (const Macro &macro, doc->definedMacros())
            env->bind(macro);
    }
}

QByteArray TypeOfExpression::preprocessedExpression(const QByteArray &utf8code) const
{
    if (utf8code.trimmed().isEmpty())
        return utf8code;

    if (! m_environment) {
        Environment *env = new Environment(); // ### cache the environment.

        QSet<QString> processed;
        processEnvironment(m_thisDocument, env, &processed);
        m_environment = QSharedPointer<Environment>(env);
    }

    Preprocessor preproc(0, m_environment.data());
    return preproc.run(QLatin1String("<expression>"), utf8code);
}

namespace CPlusPlus {

ExpressionAST *extractExpressionAST(Document::Ptr doc)
{
    if (! doc->translationUnit()->ast())
        return 0;

    return doc->translationUnit()->ast()->asExpression();
}

Document::Ptr documentForExpression(const QByteArray &utf8code)
{
    // create the expression's AST.
    Document::Ptr doc = Document::create(QLatin1String("<completion>"));
    doc->setUtf8Source(utf8code);
    doc->parse(Document::ParseExpression);
    return doc;
}

} // namespace CPlusPlus
