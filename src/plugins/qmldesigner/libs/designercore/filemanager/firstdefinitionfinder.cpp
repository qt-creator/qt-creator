// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "firstdefinitionfinder.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

FirstDefinitionFinder::FirstDefinitionFinder(const QString &text)
    : m_doc(QmlJS::Document::create(Utils::FilePath::fromString("<internal>"), QmlJS::Dialect::Qml))
{
    NanotraceHR::Tracer tracer{"first definition finder constructor",
                               category(),
                               keyValue("text", text)};

    m_doc->setSource(text);
    bool ok = m_doc->parseQml();

    if (!ok) {
        qDebug() << text;
        const QList<QmlJS::DiagnosticMessage> messages = m_doc->diagnosticMessages();
        for (const QmlJS::DiagnosticMessage &message : messages)
                qDebug() << message.message;
    }

    Q_ASSERT(ok);
}

/*!
    Finds the first object definition inside the object specified by \a offset.
    Returns the offset of the first object definition.
  */
qint32 FirstDefinitionFinder::operator()(quint32 offset)
{
    NanotraceHR::Tracer tracer{"first definition finder operator()",
                               category(),
                               keyValue("offset", offset)};

    m_offset = offset;
    m_firstObjectDefinition = nullptr;

    QmlJS::AST::Node::accept(m_doc->qmlProgram(), this);

    if (!m_firstObjectDefinition)
        return -1;

    return m_firstObjectDefinition->firstSourceLocation().offset;
}

void FirstDefinitionFinder::extractFirstObjectDefinition(QmlJS::AST::UiObjectInitializer* ast)
{
    NanotraceHR::Tracer tracer{"first definition finder extract first object definition", category()};

    if (!ast)
        return;

    for (QmlJS::AST::UiObjectMemberList *iter = ast->members; iter; iter = iter->next) {
        if (auto def = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(iter->member))
            m_firstObjectDefinition = def;
    }
}

bool FirstDefinitionFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"first definition finder visit ui object binding", category()};

    if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->identifierToken.isValid()) {
        const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;

        if (start == m_offset) {
            extractFirstObjectDefinition(ast->initializer);
            return false;
        }
    }
    return true;
}

bool FirstDefinitionFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"first definition finder visit ui object definition", category()};

    const quint32 start = ast->firstSourceLocation().offset;

    if (start == m_offset) {
        extractFirstObjectDefinition(ast->initializer);
        return false;
    }
    return true;
}

bool FirstDefinitionFinder::visit(QmlJS::AST::TemplateLiteral *ast)
{
    NanotraceHR::Tracer tracer{"first definition finder visit template literal", category()};

    QmlJS::AST::Node::accept(ast->expression, this);
    return true;
}

void FirstDefinitionFinder::throwRecursionDepthError()
{
    NanotraceHR::Tracer tracer{"first definition finder throw recursion depth error", category()};

    qWarning("Warning: Hit maximum recursion depth while visiting the AST in FirstDefinitionFinder");
}

} // namespace QmlDesigner
