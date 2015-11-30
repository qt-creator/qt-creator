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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "changeobjecttypevisitor.h"
#include <qmljs/parser/qmljsast_p.h>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlDesigner::Internal;

ChangeObjectTypeVisitor::ChangeObjectTypeVisitor(QmlDesigner::TextModifier &modifier,
                                                 quint32 nodeLocation,
                                                 const QString &newType):
    QMLRewriter(modifier),
    m_nodeLocation(nodeLocation),
    m_newType(newType)
{
}

bool ChangeObjectTypeVisitor::visit(UiObjectDefinition *ast)
{
    if (ast->firstSourceLocation().offset == m_nodeLocation) {
        replaceType(ast->qualifiedTypeNameId);
        return false;
    }

    return !didRewriting();
}

bool ChangeObjectTypeVisitor::visit(UiObjectBinding *ast)
{
    const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;

    if (start == m_nodeLocation) {
        replaceType(ast->qualifiedTypeNameId);
        return false;
    }

    return !didRewriting();
}

void ChangeObjectTypeVisitor::replaceType(UiQualifiedId *typeId)
{
    Q_ASSERT(typeId);

    const int startOffset = typeId->identifierToken.offset;
    int endOffset = typeId->identifierToken.end();
    for (UiQualifiedId *iter = typeId->next; iter; iter = iter->next)
        if (!iter->next)
            endOffset = iter->identifierToken.end();

    replace(startOffset, endOffset - startOffset, m_newType);
    setDidRewriting(true);
}
