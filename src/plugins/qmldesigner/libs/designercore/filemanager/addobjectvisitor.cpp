// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addobjectvisitor.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

AddObjectVisitor::AddObjectVisitor(QmlDesigner::TextModifier &modifier,
                                   quint32 parentLocation,
                                   std::optional<int> nodeLocation,
                                   const QString &content,
                                   Utils::span<const PropertyNameView> propertyOrder)
    : QMLRewriter(modifier)
    , m_parentLocation(parentLocation)
    , m_nodeLocation(nodeLocation)
    , m_content(content)
    , m_propertyOrder(propertyOrder)
{
    NanotraceHR::Tracer tracer{"add object visitor constructor",
                               category(),
                               NanotraceHR::keyValue("parent location", parentLocation),
                               NanotraceHR::keyValue("node location", nodeLocation),
                               NanotraceHR::keyValue("content", content)};
}

bool AddObjectVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"add object visitor visit ui object binding",
                               RewriterTracing::category()};

    if (didRewriting())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == m_parentLocation)
        insertInto(ast->initializer);

    return !didRewriting();
}

bool AddObjectVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"add object visitor visit ui object definition", category()};
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        insertInto(ast->initializer);

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddObjectVisitor::insertInto(QmlJS::AST::UiObjectInitializer *ast)
{
    NanotraceHR::Tracer tracer{"add object visitor insert into", category()};

    QmlJS::AST::UiObjectMemberList *insertAfter;
    if (m_nodeLocation.has_value())
        insertAfter = searchChildrenToInsertAfter(ast->members, m_propertyOrder, *m_nodeLocation - 1);
    else
        insertAfter = searchMemberToInsertAfter(ast->members, m_propertyOrder);

    int insertionPoint;
    int depth;
    QString textToInsert;
    if (insertAfter && insertAfter->member) {
        insertionPoint = insertAfter->member->lastSourceLocation().end();
        depth = calculateIndentDepth(insertAfter->member->lastSourceLocation());
        textToInsert += QStringLiteral("\n");
    } else {
        insertionPoint = ast->lbraceToken.end();
        depth = calculateIndentDepth(ast->lbraceToken) + indentDepth();
    }

    textToInsert += addIndentation(m_content, depth);
    replace(insertionPoint, 0, QStringLiteral("\n") + textToInsert);

    setDidRewriting(true);
}

} // namespace QmlDesigner::Internal
