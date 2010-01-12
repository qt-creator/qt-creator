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

#include "removepropertyvisitor.h"

using namespace QmlEditor::Internal;
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
        removeFrom(ast->initializer);
    }

    return !didRewriting();
}

bool RemovePropertyVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (ast->firstSourceLocation().offset == parentLocation) {
        removeFrom(ast->initializer);
    }

    return !didRewriting();
}

void RemovePropertyVisitor::removeFrom(QmlJS::AST::UiObjectInitializer *ast)
{
    UiObjectMember *previousMember = 0, *wantedMember = 0, *nextMember = 0;

    for (UiObjectMemberList *it = ast->members; it; it = it->next) {
        if (memberNameMatchesPropertyName(it->member)) {
            wantedMember = it->member;

            if (it->next)
                nextMember = it->next->member;

            break;
        }

        previousMember = it->member;
    }

    if (!wantedMember)
        return;

    int start = wantedMember->firstSourceLocation().offset;
    int end = wantedMember->lastSourceLocation().end();

    includeSurroundingWhitespace(start, end);

    replace(start, end - start, QLatin1String(""));

    setDidRewriting(true);
}

bool RemovePropertyVisitor::memberNameMatchesPropertyName(QmlJS::AST::UiObjectMember *ast) const
{
    if (UiPublicMember *publicMember = cast<UiPublicMember*>(ast))
        return publicMember->name->asString() == propertyName;
    else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(ast))
        return flatten(objectBinding->qualifiedId) == propertyName;
    else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(ast))
        return flatten(scriptBinding->qualifiedId) == propertyName;
    else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(ast))
        return flatten(arrayBinding->qualifiedId) == propertyName;
    else
        return false;
}
