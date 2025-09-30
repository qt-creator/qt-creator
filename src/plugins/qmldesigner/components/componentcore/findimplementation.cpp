// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findimplementation.h"
#include "componentcoretracing.h"

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

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

class FindImplementationVisitor : protected QmlJS::AST::Visitor
{
public:
    using Results = QList<QmlJS::SourceLocation>;

    FindImplementationVisitor(const QmlJS::Document::Ptr &doc, const QmlJS::ContextPtr &context)
        : m_document(doc)
        , m_context(context)
        , m_scopeChain(doc, context)
        , m_scopeBuilder(&m_scopeChain)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor constructor", category()};
    }

    Results operator()(const QString &name, const QString &itemId, const QmlJS::ObjectValue *typeValue)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor operator()", category()};

        m_typeName = name;
        m_itemId = itemId;
        m_typeValue = typeValue;
        m_implemenations.clear();
        if (m_document)
            QmlJS::AST::Node::accept(m_document->ast(), this);

        m_implemenations.append(m_formLocation);
        return m_implemenations;
    }

protected:
    QString textAt(const QmlJS::SourceLocation &location)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor text at", category()};

        return m_document->source().mid(location.offset, location.length);
    }

    QString textAt(const QmlJS::SourceLocation &from, const QmlJS::SourceLocation &to)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor text at", category()};

        return m_document->source().mid(from.offset, to.end() - from.begin());
    }

    void accept(QmlJS::AST::Node *node)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor accept", category()};

        QmlJS::AST::Node::accept(node, this);
    }

    using QmlJS::AST::Visitor::visit;

    bool visit(QmlJS::AST::UiPublicMember *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit ui public member", category()};

        if (node->memberType && node->memberType->name == m_typeName) {
            const QmlJS::ObjectValue *objectValue = m_context->lookupType(m_document.data(),
                                                                          QStringList(m_typeName));
            if (objectValue == m_typeValue)
                m_implemenations.append(node->typeToken);
        }
        if (QmlJS::AST::cast<QmlJS::AST::Block *>(node->statement)) {
            m_scopeBuilder.push(node);
            QmlJS::AST::Node::accept(node->statement, this);
            m_scopeBuilder.pop();
            return false;
        }
        return true;
    }

    bool visit(QmlJS::AST::UiObjectDefinition *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit ui object definition",
                                   category()};

        bool oldInside = m_insideObject;
        if (checkTypeName(node->qualifiedTypeNameId))
             m_insideObject = true;

        m_scopeBuilder.push(node);
        QmlJS::AST::Node::accept(node->initializer, this);
        m_insideObject = oldInside;
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(QmlJS::AST::UiObjectBinding *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit ui object binding", category()};

        bool oldInside = m_insideObject;
        if (checkTypeName(node->qualifiedTypeNameId))
            m_insideObject = true;

        m_scopeBuilder.push(node);
        QmlJS::AST::Node::accept(node->initializer, this);

        m_insideObject = oldInside;
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(QmlJS::AST::UiScriptBinding *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit ui script binding", category()};

        if (m_insideObject) {
            QStringList stringList = textAt(node->qualifiedId->firstSourceLocation(),
                                            node->qualifiedId->lastSourceLocation())
                                         .split(QLatin1String("."));
            const QString itemid = stringList.isEmpty() ? QString() : stringList.constFirst();

            if (itemid == m_itemId) {
                m_implemenations.append(node->statement->firstSourceLocation());
            }
        }
        if (QmlJS::AST::cast<QmlJS::AST::Block *>(node->statement)) {
            QmlJS::AST::Node::accept(node->qualifiedId, this);
            m_scopeBuilder.push(node);
            QmlJS::AST::Node::accept(node->statement, this);
            m_scopeBuilder.pop();
            return false;
        }
        return true;
    }

    bool visit(QmlJS::AST::TemplateLiteral *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit template literal", category()};

        QmlJS::AST::Node::accept(node->expression, this);
        return true;
    }

    bool visit(QmlJS::AST::IdentifierExpression *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit identifier expression",
                                   category()};

        if (node->name != m_typeName)
            return false;

        const QmlJS::ObjectValue *scope;
        const QmlJS::Value *objectValue = m_scopeChain.lookup(m_typeName, &scope);
        if (objectValue == m_typeValue)
            m_implemenations.append(node->identifierToken);
        return false;
    }

    bool visit(QmlJS::AST::FieldMemberExpression *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit field member expression",
                                   category()};

        if (node->name != m_typeName)
            return true;
        QmlJS::Evaluate evaluate(&m_scopeChain);
        const QmlJS::Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;
        const QmlJS::ObjectValue *lhsObj = lhsValue->asObjectValue();
        if (lhsObj && lhsObj->lookupMember(m_typeName, m_context) == m_typeValue)
            m_implemenations.append(node->identifierToken);
        return true;
    }

    bool visit(QmlJS::AST::FunctionDeclaration *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit function declaration",
                                   category()};

        return visit(static_cast<QmlJS::AST::FunctionExpression *>(node));
    }

    bool visit(QmlJS::AST::FunctionExpression *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit function expression",
                                   category()};

        QmlJS::AST::Node::accept(node->formals, this);
        m_scopeBuilder.push(node);
        QmlJS::AST::Node::accept(node->body, this);
        m_scopeBuilder.pop();
        return false;
    }

    bool visit(QmlJS::AST::PatternElement *node) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit pattern element", category()};

        if (node->isVariableDeclaration())
            QmlJS::AST::Node::accept(node->initializer, this);
        return false;
    }

    bool visit(QmlJS::AST::UiImport *ast) override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor visit ui import", category()};

        if (ast && ast->importId == m_typeName) {
            const QmlJS::Imports *imp = m_context->imports(m_document.data());
            if (!imp)
                return false;
            if (m_context->lookupType(m_document.data(), QStringList(m_typeName)) == m_typeValue)
                m_implemenations.append(ast->importIdToken);
        }
        return false;
    }

    void throwRecursionDepthError() override
    {
        NanotraceHR::Tracer tracer{"find implementation visitor throw recursion depth error",
                                   category()};

        qWarning(
            "Warning: Hit maximum recursion depth while visiting AST in FindImplementationVisitor");
    }
private:
    bool checkTypeName(QmlJS::AST::UiQualifiedId *id)
    {
        NanotraceHR::Tracer tracer{"find implementation visitor check type name", category()};

        for (QmlJS::AST::UiQualifiedId *qualifiedId = id; qualifiedId;
             qualifiedId = qualifiedId->next) {
            if (qualifiedId->name == m_typeName) {
                const QmlJS::ObjectValue *objectValue = m_context->lookupType(m_document.data(),
                                                                              id,
                                                                              qualifiedId->next);
                if (m_typeValue == objectValue){
                    m_formLocation = qualifiedId->identifierToken;
                    return true;
                }
            }
        }
        return false;
    }

    Results m_implemenations;
    QmlJS::SourceLocation m_formLocation;

    QmlJS::Document::Ptr m_document;
    QmlJS::ContextPtr m_context;
    QmlJS::ScopeChain m_scopeChain;
    QmlJS::ScopeBuilder m_scopeBuilder;

    QString m_typeName;
    QString m_itemId;
    const QmlJS::ObjectValue *m_typeValue = nullptr;
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

FindImplementation::FindImplementation()
{
    NanotraceHR::Tracer tracer{"find implementation constructor", category()};
}

QList<QmlJSEditor::FindReferences::Usage> FindImplementation::run(const QString &fileNameStr,
                                                                  const QString &typeName,
                                                                  const QString &itemName)
{
    NanotraceHR::Tracer tracer{"find implementation run",
                               category(),
                               keyValue("file", fileNameStr),
                               keyValue("type", typeName),
                               keyValue("item", itemName)};

    QList<QmlJSEditor::FindReferences::Usage> usages;

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    Utils::FilePath fileName = Utils::FilePath::fromString(fileNameStr);

    //Parse always the latest version of document
    QmlJS::Dialect dialect = QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName);
    QmlJS::Document::MutablePtr documentUpdate = QmlJS::Document::create(fileName, dialect);
    documentUpdate->setSource(QmlJS::ModelManagerInterface::workingCopy().source(fileName));
    if (documentUpdate->parseQml())
        modelManager->updateDocument(documentUpdate);

    QmlJS::Document::Ptr document = modelManager->snapshot().document(fileName);
    if (!document)
        return usages;

    QmlJS::Link link(modelManager->snapshot(),
                     modelManager->defaultVContext(document->language(), document),
                     modelManager->builtins(document));
    QmlJS::ContextPtr context = link();
    QmlJS::ScopeChain scopeChain(document, context);

    const QmlJS::ObjectValue *targetValue = scopeChain.context()->lookupType(document.data(),
                                                                             QStringList(typeName));

    FindImplementationVisitor visitor(document, context);

    const FindImplementationVisitor::Results results = visitor(typeName, itemName, targetValue);
    for (const QmlJS::SourceLocation &location : results) {
        usages.append(QmlJSEditor::FindReferences::Usage(fileName,
                                                         matchingLine(location.offset, document->source()),
                                                         location.startLine, location.startColumn - 1, location.length));
    }

    return usages;
}
