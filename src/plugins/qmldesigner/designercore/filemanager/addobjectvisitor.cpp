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
