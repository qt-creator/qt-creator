/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "changeobjecttypevisitor.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

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
