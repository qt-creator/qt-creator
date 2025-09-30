// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changepropertyvisitor.h"
#include "../rewriter/rewritertracing.h"

#include <signalhandlerproperty.h>

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

ChangePropertyVisitor::ChangePropertyVisitor(QmlDesigner::TextModifier &modifier,
                                             quint32 parentLocation,
                                             const QString &name,
                                             const QString &value,
                                             QmlRefactoring::PropertyType propertyType):
        QMLRewriter(modifier),
        m_parentLocation(parentLocation),
        m_name(name),
        m_value(value),
        m_propertyType(propertyType)
{
    NanotraceHR::Tracer tracer{"change property visitor constructor",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("name", name),
                               keyValue("value", value),
                               keyValue("property type", int(propertyType))};
}

bool ChangePropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"change property visitor visit ui object definition", category()};

    if (didRewriting())
        return false;

    const quint32 objectStart = ast->firstSourceLocation().offset;

    if (objectStart == m_parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        replaceInMembers(ast->initializer, m_name);
        return false;
    }

    return !didRewriting();
}

bool ChangePropertyVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"change property visitor visit ui object binding", category()};

    if (didRewriting())
        return false;

    const quint32 objectStart = ast->qualifiedTypeNameId->identifierToken.offset;

    if (objectStart == m_parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        replaceInMembers(ast->initializer, m_name);
        return false;
    }

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void ChangePropertyVisitor::replaceInMembers(QmlJS::AST::UiObjectInitializer *initializer,
                                             const QString &propertyName)
{
    NanotraceHR::Tracer tracer{"change property visitor replace in members",
                               category(),
                               keyValue("property name", propertyName)};

    QString prefix, suffix;
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx != -1) {
        prefix = propertyName.left(dotIdx);
        suffix = propertyName.mid(dotIdx + 1);
    }

    for (QmlJS::AST::UiObjectMemberList *members = initializer->members; members;
         members = members->next) {
        QmlJS::AST::UiObjectMember *member = members->member;

        // for non-grouped properties:
        if (isMatchingPropertyMember(propertyName, member)) {
            switch (m_propertyType) {
            case QmlRefactoring::ArrayBinding:
                insertIntoArray(cast<QmlJS::AST::UiArrayBinding *>(member));
                break;

            case QmlRefactoring::ObjectBinding:
                replaceMemberValue(member, false);
                break;

            case QmlRefactoring::ScriptBinding:
                replaceMemberValue(member, nextMemberOnSameLine(members));
                break;
            case QmlRefactoring::SignalHandlerOldSyntax:
                replaceMemberValue(member, nextMemberOnSameLine(members));
                break;
            case QmlRefactoring::SignalHandlerNewSyntax:
                replaceMemberValue(member, nextMemberOnSameLine(members));
                break;

            default:
                Q_ASSERT(!"Unhandled QmlRefactoring::PropertyType");
            }

            break;
        // for grouped properties:
        } else if (!prefix.isEmpty()) {
            if (auto def = cast<QmlJS::AST::UiObjectDefinition *>(member)) {
                if (toString(def->qualifiedTypeNameId) == prefix)
                    replaceInMembers(def->initializer, suffix);
            }
        }
    }
}

QString ensureBraces(const QString &source)
{
    return SignalHandlerProperty::normalizedSourceWithBraces(source);
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void ChangePropertyVisitor::replaceMemberValue(QmlJS::AST::UiObjectMember *propertyMember,
                                               bool needsSemicolon)
{
    NanotraceHR::Tracer tracer{"change property visitor replace member value",
                               category(),
                               keyValue("needs semicolon", needsSemicolon)};

    QString replacement = m_value;
    int startOffset = -1;
    int endOffset = -1;
    bool requiresBraces = false;

    if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(propertyMember)) {
        startOffset = objectBinding->qualifiedTypeNameId->identifierToken.offset;
        endOffset = objectBinding->initializer->rbraceToken.end();
    } else if (auto scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(propertyMember)) {
        startOffset = scriptBinding->statement->firstSourceLocation().offset;
        endOffset = scriptBinding->statement->lastSourceLocation().end();
    } else if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding *>(propertyMember)) {
        startOffset = arrayBinding->lbracketToken.offset;
        endOffset = arrayBinding->rbracketToken.end();
    } else if (auto sourceElement = QmlJS::AST::cast<QmlJS::AST::UiSourceElement *>(propertyMember)) {
        auto function = QmlJS::AST::cast<QmlJS::AST::FunctionDeclaration *>(
            sourceElement->sourceElement);
        startOffset = function->lbraceToken.offset;
        endOffset = function->rbraceToken.end();
        requiresBraces = true;
    } else if (auto publicMember = QmlJS::AST::cast<QmlJS::AST::UiPublicMember *>(propertyMember)) {
        if (publicMember->type == QmlJS::AST::UiPublicMember::Signal) {
            startOffset = publicMember->firstSourceLocation().offset;
            if (publicMember->semicolonToken.isValid())
                endOffset = publicMember->semicolonToken.end();
            else
                endOffset = publicMember->lastSourceLocation().end();
            replacement.prepend(QStringLiteral("signal %1 ").arg(publicMember->name));
        } else if (publicMember->statement) {
            startOffset = publicMember->statement->firstSourceLocation().offset;
            if (publicMember->semicolonToken.isValid())
                endOffset = publicMember->semicolonToken.end();
            else
                endOffset = publicMember->statement->lastSourceLocation().end();
        } else {
            startOffset = publicMember->lastSourceLocation().end();
            endOffset = startOffset;
            if (publicMember->semicolonToken.isValid())
                startOffset = publicMember->semicolonToken.offset;
            replacement.prepend(QStringLiteral(": "));
        }
    } else {
        return;
    }

    if (requiresBraces)
        replacement = ensureBraces(replacement);

    if (needsSemicolon)
        replacement += QChar::fromLatin1(';');

    replace(startOffset, endOffset - startOffset, replacement);
    setDidRewriting(true);
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool ChangePropertyVisitor::isMatchingPropertyMember(const QString &propName,
                                                     QmlJS::AST::UiObjectMember *member)
{
    NanotraceHR::Tracer tracer{"change property visitor is matching property member",
                               category(),
                               keyValue("property name", propName)};

    if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(member))
        return propName == toString(objectBinding->qualifiedId);
    else if (auto scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(member))
        return propName == toString(scriptBinding->qualifiedId);
    else if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding *>(member))
        return propName == toString(arrayBinding->qualifiedId);
    else if (auto publicMember = QmlJS::AST::cast<QmlJS::AST::UiPublicMember *>(member))
        return propName == publicMember->name;
    else if (auto uiSourceElement = QmlJS::AST::cast<QmlJS::AST::UiSourceElement *>(member)) {
        auto function = QmlJS::AST::cast<QmlJS::AST::FunctionDeclaration *>(
            uiSourceElement->sourceElement);

        return function->name == propName;
    } else
        return false;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool ChangePropertyVisitor::nextMemberOnSameLine(QmlJS::AST::UiObjectMemberList *members)
{
    NanotraceHR::Tracer tracer{"change property visitor next member on same line", category()};

    if (members && members->next && members->next->member)
        return members->next->member->firstSourceLocation().startLine == members->member->lastSourceLocation().startLine;
    else
        return false;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void ChangePropertyVisitor::insertIntoArray(QmlJS::AST::UiArrayBinding *ast)
{
    NanotraceHR::Tracer tracer{"change property visitor insert into array", category()};

    if (!ast)
        return;

    QmlJS::AST::UiObjectMember *lastMember = nullptr;
    for (QmlJS::AST::UiArrayMemberList *iter = ast->members; iter; iter = iter->next) {
        lastMember = iter->member;
    }

    if (!lastMember)
        return;

    const int insertionPoint = lastMember->lastSourceLocation().end();
    const int depth = calculateIndentDepth(lastMember->firstSourceLocation());
    const QString indentedArrayMember = addIndentation(m_value, depth);
    replace(insertionPoint, 0, QStringLiteral(",\n") + indentedArrayMember);
    setDidRewriting(true);
}
} // namespace QmlDesigner::Internal
