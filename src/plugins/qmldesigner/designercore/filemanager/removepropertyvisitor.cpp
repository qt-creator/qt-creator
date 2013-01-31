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

#include "removepropertyvisitor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlDesigner::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

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

    for (UiObjectMemberList *it = ast->members; it; it = it->next) {
        UiObjectMember *member = it->member;

        // run full name match (for ungrouped properties):
        if (memberNameMatchesPropertyName(propertyName, member))
            removeMember(member);
        // check for grouped properties:
        else if (!prefix.isEmpty()) {
            if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member)) {
                if (toString(def->qualifiedTypeNameId) == prefix)
                    removeGroupedProperty(def);
            }
        }
    }
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemovePropertyVisitor::removeGroupedProperty(UiObjectDefinition *ast)
{
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;

    const QString propName = propertyName.mid(dotIdx + 1);

    UiObjectMember *wanted = 0;
    unsigned memberCount = 0;
    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        ++memberCount;
        UiObjectMember *member = it->member;

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
void RemovePropertyVisitor::removeMember(UiObjectMember *member)
{
    int start = member->firstSourceLocation().offset;
    int end = member->lastSourceLocation().end();

    includeSurroundingWhitespace(start, end);

    replace(start, end - start, QLatin1String(""));
    setDidRewriting(true);
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool RemovePropertyVisitor::memberNameMatchesPropertyName(const QString &propertyName, UiObjectMember *ast)
{
    if (UiPublicMember *publicMember = cast<UiPublicMember*>(ast))
        return publicMember->name == propertyName;
    else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(ast))
        return toString(objectBinding->qualifiedId) == propertyName;
    else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(ast))
        return toString(scriptBinding->qualifiedId) == propertyName;
    else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(ast))
        return toString(arrayBinding->qualifiedId) == propertyName;
    else
        return false;
}
