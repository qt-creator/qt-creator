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

#include "addarraymembervisitor.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

AddArrayMemberVisitor::AddArrayMemberVisitor(TextModifier &modifier,
                                             quint32 parentLocation,
                                             const QString &propertyName,
                                             const QString &content):
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_propertyName(propertyName),
    m_content(content),
    m_convertObjectBindingIntoArrayBinding(false)
{
}

void AddArrayMemberVisitor::findArrayBindingAndInsert(const QString &propertyName, QmlJS::AST::UiObjectMemberList *ast)
{
    for (QmlJS::AST::UiObjectMemberList *iter = ast; iter; iter = iter->next) {
        if (QmlJS::AST::UiArrayBinding *arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(iter->member)) {
            if (toString(arrayBinding->qualifiedId) == propertyName)
                insertInto(arrayBinding);
        } else if (QmlJS::AST::UiObjectBinding *objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(iter->member)) {
            if (toString(objectBinding->qualifiedId) == propertyName && willConvertObjectBindingIntoArrayBinding())
                convertAndAdd(objectBinding);
        }
    }
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddArrayMemberVisitor::insertInto(QmlJS::AST::UiArrayBinding *arrayBinding)
{
    QmlJS::AST::UiObjectMember *lastMember = 0;
    for (QmlJS::AST::UiArrayMemberList *iter = arrayBinding->members; iter; iter = iter->next)
        if (iter->member)
            lastMember = iter->member;

    if (!lastMember)
        return; // an array binding cannot be empty, so there will (or should) always be a last member.

    const int insertionPoint = lastMember->lastSourceLocation().end();
    const int indentDepth = calculateIndentDepth(lastMember->firstSourceLocation());

    replace(insertionPoint, 0, QStringLiteral(",\n") + addIndentation(m_content, indentDepth));

    setDidRewriting(true);
}

void AddArrayMemberVisitor::convertAndAdd(QmlJS::AST::UiObjectBinding *objectBinding)
{
    const int indentDepth = calculateIndentDepth(objectBinding->firstSourceLocation());
    const QString arrayPrefix = QStringLiteral("[\n") + addIndentation(QString(), indentDepth);
    replace(objectBinding->qualifiedTypeNameId->identifierToken.offset, 0, arrayPrefix);
    const int insertionPoint = objectBinding->lastSourceLocation().end();
    replace(insertionPoint, 0,
            QStringLiteral(",\n")
            + addIndentation(m_content, indentDepth) + QLatin1Char('\n')
            + addIndentation(QStringLiteral("]"), indentDepth)
            );

    setDidRewriting(true);
}
