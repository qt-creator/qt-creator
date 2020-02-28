/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "findimplementation.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QDebug>

namespace {

using namespace QmlJS;

class FindImplementationVisitor: protected AST::Visitor
{
public:
    using Results = QList<SourceLocation>;

    FindImplementationVisitor(const Document::Ptr &doc, const ContextPtr &context)
        : m_document(doc)
        , m_context(context)
        , m_scopeChain(doc, context)
        , m_scopeBuilder(&m_scopeChain)
    {
    }

    Results operator()(const QString &name, const QString &itemId, const ObjectValue *typeValue)
    {
        m_typeName = name;
        m_itemId = itemId;
        m_typeValue = typeValue;
        m_implemenations.clear();
        if (m_document)
            AST::Node::accept(m_document->ast(), this);

        m_implemenations.append(m_formLocation);
        return m_implemenations;
    }

protected:
    QString textAt(const SourceLocation &location)
    {
        return m_document->source().mid(location.offset, location.length);
    }

    QString textAt(const SourceLocation &from,
                   const SourceLocation &to)
    {
        return m_document->source().mid(from.offset, to.end() - from.begin());
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using AST::Visitor::visit;

    bool visit(AST::UiPublicMember *node) override
    {
        if (node->memberType && node->memberType->name == m_typeName){
            const ObjectValue * objectValue = m_context->lookupType(m_document.data(), QStringList(m_typeName));
            if (objectValue == m_typeValue)
                m_implemenations.append(node->typeToken);
        }
        if (AST::cast<AST::Block *>(node->statement)) {
            m_scopeBuilder.push(node);
            AST::Node::accept(node->statement, this);
            m_scopeBuilder.pop();
            return false;
        }
        return true;
    }

    bool visit(AST::UiObjectDefinition *node) override
    {
        bool oldInside = m_insideObject;
        if (checkTypeName(node->qualifiedTypeNameId))
             m_insideObject = true;

        m_scopeBuilder.push(node);
        AST::Node::accept(node->initializer, this);
        m_insideObject = oldInside;
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(AST::UiObjectBinding *node) override
    {
        bool oldInside = m_insideObject;
        if (checkTypeName(node->qualifiedTypeNameId))
            m_insideObject = true;

        m_scopeBuilder.push(node);
        AST::Node::accept(node->initializer, this);

        m_insideObject = oldInside;
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(AST::UiScriptBinding *node) override
    {
        if (m_insideObject) {
            QStringList stringList = textAt(node->qualifiedId->firstSourceLocation(),
                                            node->qualifiedId->lastSourceLocation()).split(QLatin1String("."));
            const QString itemid = stringList.isEmpty() ? QString() : stringList.constFirst();

            if (itemid == m_itemId) {
                m_implemenations.append(node->statement->firstSourceLocation());
            }

        }
        if (AST::cast<AST::Block *>(node->statement)) {
            AST::Node::accept(node->qualifiedId, this);
            m_scopeBuilder.push(node);
            AST::Node::accept(node->statement, this);
            m_scopeBuilder.pop();
            return false;
        }
        return true;
    }

    bool visit(AST::IdentifierExpression *node) override
    {
        if (node->name != m_typeName)
            return false;

        const ObjectValue *scope;
        const Value *objectValue = m_scopeChain.lookup(m_typeName, &scope);
        if (objectValue == m_typeValue)
            m_implemenations.append(node->identifierToken);
        return false;
    }

    bool visit(AST::FieldMemberExpression *node) override
    {
        if (node->name != m_typeName)
            return true;
        Evaluate evaluate(&m_scopeChain);
        const Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;
        const ObjectValue *lhsObj = lhsValue->asObjectValue();
        if (lhsObj && lhsObj->lookupMember(m_typeName, m_context) == m_typeValue)
            m_implemenations.append(node->identifierToken);
        return true;
    }

    bool visit(AST::FunctionDeclaration *node) override
    {
        return visit(static_cast<AST::FunctionExpression *>(node));
    }

    bool visit(AST::FunctionExpression *node) override
    {
        AST::Node::accept(node->formals, this);
        m_scopeBuilder.push(node);
        AST::Node::accept(node->body, this);
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(AST::PatternElement *node) override
    {
    if (node->isVariableDeclaration())
            AST::Node::accept(node->initializer, this);
        return false;
    }

    bool visit(AST::UiImport *ast) override
    {
        if (ast && ast->importId == m_typeName) {
            const Imports *imp = m_context->imports(m_document.data());
            if (!imp)
                return false;
            if (m_context->lookupType(m_document.data(), QStringList(m_typeName)) == m_typeValue)
                m_implemenations.append(ast->importIdToken);
        }
        return false;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in FindImplementationVisitor");
    }
private:
    bool checkTypeName(AST::UiQualifiedId *id)
    {
        for (AST::UiQualifiedId *qualifiedId = id; qualifiedId; qualifiedId = qualifiedId->next){
            if (qualifiedId->name == m_typeName) {
                const ObjectValue *objectValue = m_context->lookupType(m_document.data(), id, qualifiedId->next);
                if (m_typeValue == objectValue){
                    m_formLocation = qualifiedId->identifierToken;
                    return true;
                }
            }
        }
        return false;
    }

    Results m_implemenations;
    SourceLocation m_formLocation;

    Document::Ptr m_document;
    ContextPtr m_context;
    ScopeChain m_scopeChain;
    ScopeBuilder m_scopeBuilder;

    QString m_typeName;
    QString m_itemId;
    const ObjectValue *m_typeValue = nullptr;
    bool m_insideObject = false;
};


QString matchingLine(unsigned position, const QString &source)
{
    int start = source.lastIndexOf(QLatin1Char('\n'), position);
    start += 1;
    int end = source.indexOf(QLatin1Char('\n'), position);

    return source.mid(start, end - start);
}

} //namespace


FindImplementation::FindImplementation() = default;

QList<QmlJSEditor::FindReferences::Usage> FindImplementation::run(const QString &fileName,
                                                                  const QString &typeName,
                                                                  const QString &itemName)
{
    QList<QmlJSEditor::FindReferences::Usage> usages;

    QmlJS::ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    //Parse always the latest version of document
    QmlJS::Dialect dialect = QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName);
    QmlJS::Document::MutablePtr documentUpdate = QmlJS::Document::create(fileName, dialect);
    documentUpdate->setSource(QmlJS::ModelManagerInterface::workingCopy().source(fileName));
    if (documentUpdate->parseQml())
        modelManager->updateDocument(documentUpdate);

    Document::Ptr document = modelManager->snapshot().document(fileName);
    if (!document)
        return usages;

    QmlJS::Link link(modelManager->snapshot(),
                     modelManager->defaultVContext(document->language(), document),
                     modelManager->builtins(document));
    ContextPtr context = link();
    ScopeChain scopeChain(document, context);

    const ObjectValue *targetValue = scopeChain.context()->lookupType(document.data(), QStringList(typeName));

    FindImplementationVisitor visitor(document, context);

    FindImplementationVisitor::Results results = visitor(typeName, itemName, targetValue);
    foreach (const SourceLocation &location, results) {
        usages.append(QmlJSEditor::FindReferences::Usage(fileName,
                                                         matchingLine(location.offset, document->source()),
                                                         location.startLine, location.startColumn - 1, location.length));
    }

    return usages;
}
