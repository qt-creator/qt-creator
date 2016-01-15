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

#include "addobjectvisitor.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

AddObjectVisitor::AddObjectVisitor(QmlDesigner::TextModifier &modifier,
                                   quint32 parentLocation,
                                   const QString &content,
                                   const PropertyNameList &propertyOrder):
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_content(content),
    m_propertyOrder(propertyOrder)
{
}

bool AddObjectVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == m_parentLocation)
        insertInto(ast->initializer);

    return !didRewriting();
}

bool AddObjectVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        insertInto(ast->initializer);

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddObjectVisitor::insertInto(QmlJS::AST::UiObjectInitializer *ast)
{
    QmlJS::AST::UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members, m_propertyOrder);

    int insertionPoint;
    int depth;
    QString textToInsert;
    if (insertAfter && insertAfter->member) {
        insertionPoint = insertAfter->member->lastSourceLocation().end();
        depth = calculateIndentDepth(insertAfter->member->lastSourceLocation());
        textToInsert += QStringLiteral("\n");
    } else {
        insertionPoint = ast->lbraceToken.end();
        depth = calculateIndentDepth(ast->lbraceToken) + indentDepth();
    }

    textToInsert += addIndentation(m_content, depth);
    replace(insertionPoint, 0, QStringLiteral("\n") + textToInsert);

    setDidRewriting(true);
}
