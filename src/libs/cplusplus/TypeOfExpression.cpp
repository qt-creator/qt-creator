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

#include "TypeOfExpression.h"

#include "LookupContext.h"
#include "ResolveExpression.h"
#include "pp.h"

#include <cplusplus/AST.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/TranslationUnit.h>

#include <QSet>

using namespace CPlusPlus;

TypeOfExpression::TypeOfExpression():
    m_ast(0),
    m_scope(0),
    m_expandTemplates(false)
{
}

void TypeOfExpression::init(Document::Ptr thisDocument, const Snapshot &snapshot,
                            CreateBindings::Ptr bindings,
                            const QSet<const Declaration *> &autoDeclarationsBeingResolved)
{
    m_thisDocument = thisDocument;
    m_snapshot = snapshot;
    m_ast = 0;
    m_scope = 0;
    m_lookupContext = LookupContext();

    Q_ASSERT(m_bindings.isNull());
    m_bindings = bindings;
    if (m_bindings.isNull())
        m_bindings = CreateBindings::Ptr(new CreateBindings(thisDocument, snapshot));

    m_environment.clear();
    m_autoDeclarationsBeingResolved = autoDeclarationsBeingResolved;
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

    m_bindings->addExpressionDocument(document);
    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot, m_bindings);
    Q_ASSERT(!m_bindings.isNull());
    m_lookupContext.setExpandTemplates(m_expandTemplates);

    ResolveExpression resolve(m_lookupContext, m_autoDeclarationsBeingResolved);
    return resolve(m_ast, scope);
}

QList<LookupItem> TypeOfExpression::reference(ExpressionAST *expression,
                                              Document::Ptr document,
                                              Scope *scope)
{
    m_ast = expression;

    m_scope = scope;

    m_bindings->addExpressionDocument(document);
    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot, m_bindings);
    Q_ASSERT(!m_bindings.isNull());
    m_lookupContext.setExpandTemplates(m_expandTemplates);

    ResolveExpression resolve(m_lookupContext, m_autoDeclarationsBeingResolved);
    return resolve.reference(m_ast, scope);
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

        foreach (const Document::Include &incl, doc->resolvedIncludes())
            processEnvironment(m_snapshot.document(incl.resolvedFileName()), env, processed);

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
