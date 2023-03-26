// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    return nullptr;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemoveUIObjectMemberVisitor::extendToLeadingOrTrailingComma(QmlJS::AST::UiArrayBinding *parentArray,
                                                                 QmlJS::AST::UiObjectMember *ast,
                                                                 int &start,
                                                                 int &end) const
{
    QmlJS::AST::UiArrayMemberList *currentMember = nullptr;
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
