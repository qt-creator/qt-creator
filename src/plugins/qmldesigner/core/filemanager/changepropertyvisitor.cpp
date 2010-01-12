/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <qmljsast_p.h>
#include <qmljsengine_p.h>

#include "changepropertyvisitor.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlEditor;
using namespace QmlEditor::Internal;

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
}

bool ChangePropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    const quint32 objectStart = ast->firstSourceLocation().offset;

    if (objectStart == m_parentLocation) {
        replaceInMembers(ast->initializer);
        return false;
    }

    return !didRewriting();
}

bool ChangePropertyVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    const quint32 objectStart = ast->qualifiedTypeNameId->identifierToken.offset;

    if (objectStart == m_parentLocation) {
        replaceInMembers(ast->initializer);
        return false;
    }

    return !didRewriting();
}

void ChangePropertyVisitor::replaceInMembers(UiObjectInitializer *initializer)
{
    for (UiObjectMemberList *members = initializer->members; members; members = members->next) {
        UiObjectMember *propertyMember = members->member;

        if (isMatchingPropertyMember(propertyMember)) {
            switch (m_propertyType) {
            case QmlRefactoring::ArrayBinding:
                insertIntoArray(cast<UiArrayBinding*>(propertyMember));
                break;

            case QmlRefactoring::ObjectBinding:
                replaceMemberValue(propertyMember, false);
                break;

            case QmlRefactoring::ScriptBinding:
                replaceMemberValue(propertyMember, nextMemberOnSameLine(members));
                break;

            default:
                Q_ASSERT(!"Unhandled QmlRefactoring::PropertyType");
            }

            break;
        }
    }
}

void ChangePropertyVisitor::replaceMemberValue(UiObjectMember *propertyMember, bool needsSemicolon)
{
    QString replacement = m_value;
    int startOffset = -1;
    int endOffset = -1;
    if (UiObjectBinding *objectBinding = AST::cast<UiObjectBinding *>(propertyMember)) {
        startOffset = objectBinding->qualifiedTypeNameId->identifierToken.offset;
        endOffset = objectBinding->initializer->rbraceToken.end();
    } else if (UiScriptBinding *scriptBinding = AST::cast<UiScriptBinding *>(propertyMember)) {
        startOffset = scriptBinding->statement->firstSourceLocation().offset;
        endOffset = scriptBinding->statement->lastSourceLocation().end();
    } else if (UiArrayBinding *arrayBinding = AST::cast<UiArrayBinding *>(propertyMember)) {
        startOffset = arrayBinding->lbracketToken.offset;
        endOffset = arrayBinding->rbracketToken.end();
    } else if (UiPublicMember *publicMember = AST::cast<UiPublicMember*>(propertyMember)) {
        if (publicMember->expression) {
            startOffset = publicMember->expression->firstSourceLocation().offset;
            if (publicMember->semicolonToken.isValid())
                endOffset = publicMember->semicolonToken.end();
            else
                endOffset = publicMember->expression->lastSourceLocation().offset;
        } else {
            startOffset = publicMember->lastSourceLocation().end();
            endOffset = startOffset;
            if (publicMember->semicolonToken.isValid())
                startOffset = publicMember->semicolonToken.offset;
            replacement.prepend(QLatin1String(": "));
        }
    } else {
        return;
    }

    if (needsSemicolon)
        replacement += ';';

    replace(startOffset, endOffset - startOffset, replacement);
    setDidRewriting(true);
}

bool ChangePropertyVisitor::isMatchingPropertyMember(QmlJS::AST::UiObjectMember *member) const
{
    if (UiObjectBinding *objectBinding = AST::cast<UiObjectBinding *>(member)) {
        return m_name == flatten(objectBinding->qualifiedId);
    } else if (UiScriptBinding *scriptBinding = AST::cast<UiScriptBinding *>(member)) {
        return m_name == flatten(scriptBinding->qualifiedId);
    } else if (UiArrayBinding *arrayBinding = AST::cast<UiArrayBinding *>(member)) {
        return m_name == flatten(arrayBinding->qualifiedId);
    } else if (UiPublicMember *publicMember = AST::cast<UiPublicMember *>(member)) {
        return m_name == publicMember->name->asString();
    } else {
        return false;
    }
}

bool ChangePropertyVisitor::nextMemberOnSameLine(UiObjectMemberList *members)
{
    if (members && members->next && members->next->member) {
        return members->next->member->firstSourceLocation().startLine == members->member->lastSourceLocation().startLine;
    } else {
        return false;
    }
}

void ChangePropertyVisitor::insertIntoArray(QmlJS::AST::UiArrayBinding *ast)
{
    if (!ast)
        return;

    UiObjectMember *lastMember = 0;
    for (UiArrayMemberList *iter = ast->members; iter; iter = iter->next) {
    }

    if (!lastMember)
        return;

    const int insertionPoint = lastMember->lastSourceLocation().end();
    const int indentDepth = calculateIndentDepth(lastMember->firstSourceLocation());
    const QString indentedArrayMember = addIndentation(m_value, indentDepth);
    replace(insertionPoint, 0, QLatin1String(",\n") + indentedArrayMember);
    setDidRewriting(true);
}
