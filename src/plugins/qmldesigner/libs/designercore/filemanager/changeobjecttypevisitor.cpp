// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeobjecttypevisitor.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

ChangeObjectTypeVisitor::ChangeObjectTypeVisitor(QmlDesigner::TextModifier &modifier,
                                                 quint32 nodeLocation,
                                                 const QString &newType):
    QMLRewriter(modifier),
    m_nodeLocation(nodeLocation),
    m_newType(newType)
{
    NanotraceHR::Tracer tracer{"change object type visitor constructor",
                               category(),
                               NanotraceHR::keyValue("node location", nodeLocation),
                               NanotraceHR::keyValue("new type", newType)};
}

bool ChangeObjectTypeVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"change object type visitor visit ui object definition", category()};

    if (ast->firstSourceLocation().offset == m_nodeLocation) {
        replaceType(ast->qualifiedTypeNameId);
        return false;
    }

    return !didRewriting();
}

bool ChangeObjectTypeVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"change object type visitor visit ui object binding", category()};

    const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;

    if (start == m_nodeLocation) {
        replaceType(ast->qualifiedTypeNameId);
        return false;
    }

    return !didRewriting();
}

void ChangeObjectTypeVisitor::replaceType(QmlJS::AST::UiQualifiedId *typeId)
{
    NanotraceHR::Tracer tracer{"change object type visitor replace type", category()};

    Q_ASSERT(typeId);

    const int startOffset = typeId->identifierToken.offset;
    int endOffset = typeId->identifierToken.end();
    for (QmlJS::AST::UiQualifiedId *iter = typeId->next; iter; iter = iter->next)
        if (!iter->next)
            endOffset = iter->identifierToken.end();

    replace(startOffset, endOffset - startOffset, m_newType);
    setDidRewriting(true);
}

} // namespace QmlDesigner::Internal
