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

#include "addobjectvisitor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

AddObjectVisitor::AddObjectVisitor(QmlDesigner::TextModifier &modifier,
                                   quint32 parentLocation,
                                   const QString &content,
                                   const QStringList &propertyOrder):
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
    UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members, m_propertyOrder);

    int insertionPoint;
    int depth;
    QString textToInsert;
    if (insertAfter && insertAfter->member) {
        insertionPoint = insertAfter->member->lastSourceLocation().end();
        depth = calculateIndentDepth(insertAfter->member->lastSourceLocation());
        textToInsert += QLatin1String("\n");
    } else {
        insertionPoint = ast->lbraceToken.end();
        depth = calculateIndentDepth(ast->lbraceToken) + indentDepth();
    }

    textToInsert += addIndentation(m_content, depth);
    replace(insertionPoint, 0, QLatin1String("\n") + textToInsert);

    setDidRewriting(true);
}
