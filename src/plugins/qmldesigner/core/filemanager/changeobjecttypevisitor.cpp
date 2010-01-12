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

#include "changeobjecttypevisitor.h"
#include <qmljsast_p.h>
#include <qmljsengine_p.h>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlEditor::Internal;

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
