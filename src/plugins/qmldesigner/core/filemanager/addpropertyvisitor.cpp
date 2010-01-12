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

#include "addpropertyvisitor.h"

using namespace QmlEditor;
using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

AddPropertyVisitor::AddPropertyVisitor(QmlDesigner::TextModifier &modifier,
                                       quint32 parentLocation,
                                       const QString &name,
                                       const QString &value,
                                       QmlRefactoring::PropertyType propertyType,
                                       const QStringList &propertyOrder):
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_name(name),
    m_value(value),
    m_propertyType(propertyType),
    m_propertyOrder(propertyOrder)
{
}

bool AddPropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation) {
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
        addInMembers(ast->initializer);
        return false;
    }

    return !didRewriting();
}

void AddPropertyVisitor::addInMembers(QmlJS::AST::UiObjectInitializer *initializer)
{
    UiObjectMemberList *insertAfter = searchMemberToInsertAfter(initializer->members, m_name, m_propertyOrder);
    SourceLocation endOfPreviousMember;
    SourceLocation startOfNextMember;
    unsigned indentDepth;

    if (insertAfter == 0 || insertAfter->member == 0) {
        // insert as first member
        endOfPreviousMember = initializer->lbraceToken;

        if (initializer->members && initializer->members->member)
            startOfNextMember = initializer->members->member->firstSourceLocation();
        else
            startOfNextMember = initializer->rbraceToken;

        indentDepth = calculateIndentDepth(endOfPreviousMember) + 4;
    } else {
        endOfPreviousMember = insertAfter->member->lastSourceLocation();

        if (insertAfter->next && insertAfter->next->member)
            startOfNextMember = insertAfter->next->member->firstSourceLocation();
        else
            startOfNextMember = initializer->rbraceToken;

        indentDepth = calculateIndentDepth(endOfPreviousMember);
    }

    QString newPropertyTemplate;
    switch (m_propertyType) {
    case QmlRefactoring::ArrayBinding:
        newPropertyTemplate = QLatin1String("%1: [\n%2\n]");
        m_value = addIndentation(m_value, 4);
        break;

    case QmlRefactoring::ObjectBinding:
        newPropertyTemplate = QLatin1String("%1: %2");
        break;

    case QmlRefactoring::ScriptBinding:
        newPropertyTemplate = QLatin1String("%1: %2");
        break;

    default:
        Q_ASSERT(!"unknown property type");
    }

    const bool isOneLiner = endOfPreviousMember.startLine == startOfNextMember.startLine;
    if (isOneLiner)
        newPropertyTemplate += QLatin1Char('\n');

    const QString newPropertyText = addIndentation(newPropertyTemplate.arg(m_name, m_value), indentDepth);
    replace(endOfPreviousMember.end(), 0, QLatin1Char('\n') + newPropertyText);

    setDidRewriting(true);
}
