/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "removeuiobjectmembervisitor.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

RemoveUIObjectMemberVisitor::RemoveUIObjectMemberVisitor(TextModifier &modifier,
                                                         quint32 objectLocation):
    QMLRewriter(modifier),
    objectLocation(objectLocation)
{
}

bool RemoveUIObjectMemberVisitor::preVisit(QmlJS::AST::Node *ast)
{
    parents.push(ast);

    return true;
}

void RemoveUIObjectMemberVisitor::postVisit(QmlJS::AST::Node *)
{
    parents.pop();
}

bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiPublicMember *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiObjectDefinition *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiSourceElement *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiObjectBinding *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiScriptBinding *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiArrayBinding *ast) { return visitObjectMember(ast); }

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool RemoveUIObjectMemberVisitor::visitObjectMember(QmlJS::AST::UiObjectMember *ast)
{
    const quint32 memberStart = ast->firstSourceLocation().offset;

    if (memberStart == objectLocation) {
        // found it
        int start = objectLocation;
        int end = ast->lastSourceLocation().end();

        if (QmlJS::AST::UiArrayBinding *parentArray = containingArray())
            extendToLeadingOrTrailingComma(parentArray, ast, start, end);
        else
            includeSurroundingWhitespace(start, end);

        includeLeadingEmptyLine(start);
        replace(start, end - start, QStringLiteral(""));

        setDidRewriting(true);

        return false;
    } else if (ast->lastSourceLocation().end() <= objectLocation) {
        // optimization: if the location of the object-to-be-removed is not inside the current member, skip any children
        return false;
    } else {
        // only visit children if the rewriting isn't done yet.
        return !didRewriting();
    }
}

QmlJS::AST::UiArrayBinding *RemoveUIObjectMemberVisitor::containingArray() const
{
    if (parents.size() > 2) {
        if (QmlJS::AST::cast<QmlJS::AST::UiArrayMemberList*>(parents[parents.size() - 2]))
            return QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(parents[parents.size() - 3]);
    }

    return 0;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemoveUIObjectMemberVisitor::extendToLeadingOrTrailingComma(QmlJS::AST::UiArrayBinding *parentArray,
                                                                 QmlJS::AST::UiObjectMember *ast,
                                                                 int &start,
                                                                 int &end) const
{
    QmlJS::AST::UiArrayMemberList *currentMember = 0;
    for (QmlJS::AST::UiArrayMemberList *it = parentArray->members; it; it = it->next) {
        if (it->member == ast) {
            currentMember = it;
            break;
        }
    }

    if (!currentMember)
        return;

    if (currentMember->commaToken.isValid()) {
        // leading comma
        start = currentMember->commaToken.offset;
        if (includeSurroundingWhitespace(start, end))
            --end;
    } else if (currentMember->next && currentMember->next->commaToken.isValid()) {
        // trailing comma
        end = currentMember->next->commaToken.end();
        includeSurroundingWhitespace(start, end);
    } else {
        // array with 1 element, so remove the complete binding
        start = parentArray->firstSourceLocation().offset;
        end = parentArray->lastSourceLocation().end();
        includeSurroundingWhitespace(start, end);
    }
}
