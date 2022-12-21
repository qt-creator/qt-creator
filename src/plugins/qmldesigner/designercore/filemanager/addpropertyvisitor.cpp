// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addpropertyvisitor.h"

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner {
namespace Internal {

AddPropertyVisitor::AddPropertyVisitor(TextModifier &modifier,
                                       quint32 parentLocation,
                                       const PropertyName &name,
                                       const QString &value,
                                       QmlRefactoring::PropertyType propertyType,
                                       const PropertyNameList &propertyOrder,
                                       const TypeName &dynamicTypeName) :
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_name(name),
    m_value(value),
    m_propertyType(propertyType),
    m_propertyOrder(propertyOrder),
    m_dynamicTypeName(dynamicTypeName)
{
}

bool AddPropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        addInMembers(ast->initializer);
        return false;
    }

    return !didRewriting();
}

bool AddPropertyVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == m_parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        addInMembers(ast->initializer);
        return false;
    }

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddPropertyVisitor::addInMembers(QmlJS::AST::UiObjectInitializer *initializer)
{
    QmlJS::AST::UiObjectMemberList *insertAfter = searchMemberToInsertAfter(initializer->members, m_name, m_propertyOrder);
    QmlJS::SourceLocation endOfPreviousMember;
    QmlJS::SourceLocation startOfNextMember;
    bool previousMemberSemicolon = false;
    unsigned depth;

    if (insertAfter == nullptr || insertAfter->member == nullptr) {
        // insert as first member
        endOfPreviousMember = initializer->lbraceToken;

        if (initializer->members && initializer->members->member)
            startOfNextMember = initializer->members->member->firstSourceLocation();
        else
            startOfNextMember = initializer->rbraceToken;

        depth = calculateIndentDepth(endOfPreviousMember) + indentDepth();
    } else {
        endOfPreviousMember = insertAfter->member->lastSourceLocation();

        // Find out if the previous members ends with semicolon.
        if (auto member = insertAfter->member) {
            auto hasStatement = [member]() -> QmlJS::AST::ExpressionStatement * {
                if (auto m = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(member))
                    return QmlJS::AST::cast<QmlJS::AST::ExpressionStatement *>(m->statement);
                if (auto m = QmlJS::AST::cast<QmlJS::AST::UiPublicMember *>(member))
                    return QmlJS::AST::cast<QmlJS::AST::ExpressionStatement *>(m->statement);
                return nullptr;
            };

            if (auto stmt = hasStatement()) {
                previousMemberSemicolon = stmt->semicolonToken.isValid()
                                          && stmt->semicolonToken.length > 0;
            } else {
                previousMemberSemicolon = endOfPreviousMember.isValid();
            }
        } else {
            previousMemberSemicolon = endOfPreviousMember.isValid();
        }

        if (insertAfter->next && insertAfter->next->member)
            startOfNextMember = insertAfter->next->member->firstSourceLocation();
        else
            startOfNextMember = initializer->rbraceToken;

        depth = calculateIndentDepth(endOfPreviousMember);
    }
    const bool isOneLiner = endOfPreviousMember.startLine == startOfNextMember.startLine;
    bool needsPreceedingSemicolon = false;
    bool needsTrailingSemicolon = false;

    if (isOneLiner) {
        if (insertAfter == nullptr) { // we're inserting after an lbrace
            if (initializer->members) { // we're inserting before a member (and not the rbrace)
                needsTrailingSemicolon = m_propertyType == QmlRefactoring::ScriptBinding;
            }
        } else { // we're inserting after a member, not after the lbrace
            if (previousMemberSemicolon) {
                if (insertAfter->next && insertAfter->next->member) { // and the after us there is a member, not an rbrace, so:
                    needsTrailingSemicolon = m_propertyType == QmlRefactoring::ScriptBinding;
                }
            } else { // there is no semicolon after the previous member (probably because there is an rbrace after us/it, so:
                needsPreceedingSemicolon = true;
            }
        }
    }

    QString newPropertyTemplate;
    switch (m_propertyType) {
    case QmlRefactoring::ArrayBinding:
        newPropertyTemplate = QStringLiteral("%1: [\n%2\n]");
        m_value = addIndentation(m_value, 4);
        break;

    case QmlRefactoring::ObjectBinding:
        newPropertyTemplate = QStringLiteral("%1: %2");
        break;

    case QmlRefactoring::ScriptBinding:
        newPropertyTemplate = QStringLiteral("%1: %2");
        break;

    default:
        Q_ASSERT(!"unknown property type");
    }

    if (!m_dynamicTypeName.isEmpty()) {
        if (m_dynamicTypeName == "signal") {
            newPropertyTemplate = "signal %1%2";
        } else {
            newPropertyTemplate.prepend(
                QStringLiteral("property %1 ").arg(QString::fromUtf8(m_dynamicTypeName)));
        }
    }

    if (isOneLiner) {
        if (needsPreceedingSemicolon)
            newPropertyTemplate.prepend(QLatin1Char(';'));
        newPropertyTemplate.prepend(QLatin1Char(' '));
        if (needsTrailingSemicolon)
            newPropertyTemplate.append(QLatin1Char(';'));
        depth = 0;
    } else {
        newPropertyTemplate.prepend(QLatin1Char('\n'));
    }

    const QString newPropertyText = addIndentation(newPropertyTemplate.arg(QString::fromLatin1(m_name), m_value), depth);
    replace(endOfPreviousMember.end(), 0, newPropertyText);

    setDidRewriting(true);
}

} // namespace Internal
} // namespace QmlDesigner
