// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addarraymembervisitor.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

AddArrayMemberVisitor::AddArrayMemberVisitor(TextModifier &modifier,
                                             quint32 parentLocation,
                                             const QString &propertyName,
                                             const QString &content):
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_propertyName(propertyName),
    m_content(content),
    m_convertObjectBindingIntoArrayBinding(false)
{
    NanotraceHR::Tracer tracer{"add array member visitor constructor",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("property name", propertyName),
                               keyValue("content", content)};
}

void AddArrayMemberVisitor::findArrayBindingAndInsert(const QString &propertyName, QmlJS::AST::UiObjectMemberList *ast)
{
    NanotraceHR::Tracer tracer{"add array member visitor find array binding and insert",
                               category(),
                               keyValue("property name", propertyName)};

    for (QmlJS::AST::UiObjectMemberList *iter = ast; iter; iter = iter->next) {
        if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(iter->member)) {
            if (toString(arrayBinding->qualifiedId) == propertyName)
                insertInto(arrayBinding);
        } else if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(iter->member)) {
            if (toString(objectBinding->qualifiedId) == propertyName && willConvertObjectBindingIntoArrayBinding())
                convertAndAdd(objectBinding);
        }
    }
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"add array member visitor visit ui object binding", category()};

    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"add array member visitor visit ui object definition", category()};

    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddArrayMemberVisitor::insertInto(QmlJS::AST::UiArrayBinding *arrayBinding)
{
    NanotraceHR::Tracer tracer{"add array member visitor insert into", category()};

    QmlJS::AST::UiObjectMember *lastMember = nullptr;
    for (QmlJS::AST::UiArrayMemberList *iter = arrayBinding->members; iter; iter = iter->next)
        if (iter->member)
            lastMember = iter->member;

    if (!lastMember)
        return; // an array binding cannot be empty, so there will (or should) always be a last member.

    const int insertionPoint = lastMember->lastSourceLocation().end();
    const int indentDepth = calculateIndentDepth(lastMember->firstSourceLocation());

    replace(insertionPoint, 0, QStringLiteral(",\n") + addIndentation(m_content, indentDepth));

    setDidRewriting(true);
}

void AddArrayMemberVisitor::convertAndAdd(QmlJS::AST::UiObjectBinding *objectBinding)
{
    NanotraceHR::Tracer tracer{"add array member visitor convert and add", category()};

    const int indentDepth = calculateIndentDepth(objectBinding->firstSourceLocation());
    const QString arrayPrefix = QStringLiteral("[\n") + addIndentation(QString(), indentDepth);
    replace(objectBinding->qualifiedTypeNameId->identifierToken.offset, 0, arrayPrefix);
    const int insertionPoint = objectBinding->lastSourceLocation().end();
    replace(insertionPoint, 0,
            QStringLiteral(",\n")
            + addIndentation(m_content, indentDepth) + QLatin1Char('\n')
            + addIndentation(QStringLiteral("]"), indentDepth)
            );

    setDidRewriting(true);
}

} // namespace QmlDesigner::Internal
