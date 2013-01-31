/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addpropertyvisitor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
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
    UiObjectMemberList *insertAfter = searchMemberToInsertAfter(initializer->members, m_name, m_propertyOrder);
    SourceLocation endOfPreviousMember;
    SourceLocation startOfNextMember;
    unsigned depth;

    if (insertAfter == 0 || insertAfter->member == 0) {
        // insert as first member
        endOfPreviousMember = initializer->lbraceToken;

        if (initializer->members && initializer->members->member)
            startOfNextMember = initializer->members->member->firstSourceLocation();
        else
            startOfNextMember = initializer->rbraceToken;

        depth = calculateIndentDepth(endOfPreviousMember) + indentDepth();
    } else {
        endOfPreviousMember = insertAfter->member->lastSourceLocation();

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
        if (insertAfter == 0) { // we're inserting after an lbrace
            if (initializer->members) { // we're inserting before a member (and not the rbrace)
                needsTrailingSemicolon = m_propertyType == QmlRefactoring::ScriptBinding;
            }
        } else { // we're inserting after a member, not after the lbrace
            if (endOfPreviousMember.isValid()) { // there already is a semicolon after the previous member
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

    const QString newPropertyText = addIndentation(newPropertyTemplate.arg(m_name, m_value), depth);
    replace(endOfPreviousMember.end(), 0, newPropertyText);

    setDidRewriting(true);
}
