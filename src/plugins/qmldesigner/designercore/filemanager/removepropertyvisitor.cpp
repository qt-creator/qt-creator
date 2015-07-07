/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "removepropertyvisitor.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner::Internal;

RemovePropertyVisitor::RemovePropertyVisitor(QmlDesigner::TextModifier &modifier,
                                             quint32 parentLocation,
                                             const QString &propertyName):
    QMLRewriter(modifier),
    parentLocation(parentLocation),
    propertyName(propertyName)
{
}

bool RemovePropertyVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (ast->firstSourceLocation().offset == parentLocation) {
        //this condition is wrong for the UiObjectBinding case, but we keep it
        //since we are paranoid until the release is done.
        // FIXME: change this to use the QmlJS::Rewriter class
        removeFrom(ast->initializer);
    }

    if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->identifierToken.offset == parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        removeFrom(ast->initializer);
    }

    return !didRewriting();
}

bool RemovePropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (ast->firstSourceLocation().offset == parentLocation) {
        // FIXME: change this to use the QmlJS::Rewriter class
        removeFrom(ast->initializer);
    }

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemovePropertyVisitor::removeFrom(QmlJS::AST::UiObjectInitializer *ast)
{
    QString prefix;
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx != -1)
        prefix = propertyName.left(dotIdx);

    for (QmlJS::AST::UiObjectMemberList *it = ast->members; it; it = it->next) {
        QmlJS::AST::UiObjectMember *member = it->member;

        // run full name match (for ungrouped properties):
        if (memberNameMatchesPropertyName(propertyName, member)) {
            removeMember(member);
        // check for grouped properties:
        } else if (!prefix.isEmpty()) {
            if (QmlJS::AST::UiObjectDefinition *def = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(member)) {
                if (toString(def->qualifiedTypeNameId) == prefix)
                    removeGroupedProperty(def);
            }
        }
    }
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemovePropertyVisitor::removeGroupedProperty(QmlJS::AST::UiObjectDefinition *ast)
{
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;

    const QString propName = propertyName.mid(dotIdx + 1);

    QmlJS::AST::UiObjectMember *wanted = 0;
    unsigned memberCount = 0;
    for (QmlJS::AST::UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        ++memberCount;
        QmlJS::AST::UiObjectMember *member = it->member;

        if (!wanted && memberNameMatchesPropertyName(propName, member))
            wanted = member;
    }

    if (!wanted)
        return;
    if (memberCount == 1)
        removeMember(ast);
    else
        removeMember(wanted);
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemovePropertyVisitor::removeMember(QmlJS::AST::UiObjectMember *member)
{
    int start = member->firstSourceLocation().offset;
    int end = member->lastSourceLocation().end();

    includeSurroundingWhitespace(start, end);

    replace(start, end - start, QStringLiteral(""));
    setDidRewriting(true);
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool RemovePropertyVisitor::memberNameMatchesPropertyName(const QString &propertyName, QmlJS::AST::UiObjectMember *ast)
{
    if (QmlJS::AST::UiPublicMember *publicMember = QmlJS::AST::cast<QmlJS::AST::UiPublicMember*>(ast))
        return publicMember->name == propertyName;
    else if (QmlJS::AST::UiObjectBinding *objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(ast))
        return toString(objectBinding->qualifiedId) == propertyName;
    else if (QmlJS::AST::UiScriptBinding *scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding*>(ast))
        return toString(scriptBinding->qualifiedId) == propertyName;
    else if (QmlJS::AST::UiArrayBinding *arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(ast))
        return toString(arrayBinding->qualifiedId) == propertyName;
    else
        return false;
}
