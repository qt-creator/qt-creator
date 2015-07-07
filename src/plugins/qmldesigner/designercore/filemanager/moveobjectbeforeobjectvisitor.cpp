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

#include "moveobjectbeforeobjectvisitor.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>

using namespace QmlDesigner::Internal;
using namespace QmlDesigner;

MoveObjectBeforeObjectVisitor::MoveObjectBeforeObjectVisitor(TextModifier &modifier,
                                                             quint32 movingObjectLocation,
                                                             bool inDefaultProperty):
    QMLRewriter(modifier),
    movingObjectLocation(movingObjectLocation),
    inDefaultProperty(inDefaultProperty),
    toEnd(true),
    beforeObjectLocation(0)
{}

MoveObjectBeforeObjectVisitor::MoveObjectBeforeObjectVisitor(TextModifier &modifier,
                                                             quint32 movingObjectLocation,
                                                             quint32 beforeObjectLocation,
                                                             bool inDefaultProperty):
    QMLRewriter(modifier),
    movingObjectLocation(movingObjectLocation),
    inDefaultProperty(inDefaultProperty),
    toEnd(false),
    beforeObjectLocation(beforeObjectLocation)
{}

bool MoveObjectBeforeObjectVisitor::operator ()(QmlJS::AST::UiProgram *ast)
{
    movingObject = 0;
    beforeObject = 0;
    movingObjectParents.clear();

    QMLRewriter::operator ()(ast);

    if (foundEverything())
        doMove();

    return didRewriting();
}

bool MoveObjectBeforeObjectVisitor::preVisit(QmlJS::AST::Node *ast)
{ if (ast) parents.push(ast); return true; }

void MoveObjectBeforeObjectVisitor::postVisit(QmlJS::AST::Node *ast)
{ if (ast) parents.pop(); }

bool MoveObjectBeforeObjectVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (foundEverything())
        return false;

    const quint32 start = ast->firstSourceLocation().offset;
    if (start == movingObjectLocation) {
        movingObject = ast;
        movingObjectParents = parents;
        movingObjectParents.pop();
    } else if (!toEnd && start == beforeObjectLocation) {
        beforeObject = ast;
    }

    if (movingObjectLocation < start)
        return false;
    else if (!toEnd && beforeObjectLocation < start)
        return false;
    else if (foundEverything())
        return false;
    else
        return true;
}

void MoveObjectBeforeObjectVisitor::doMove()
{
    Q_ASSERT(movingObject);
    Q_ASSERT(!movingObjectParents.isEmpty());

    TextModifier::MoveInfo moveInfo;
    QmlJS::AST::Node *parent = movingObjectParent();
    QmlJS::AST::UiArrayMemberList *arrayMember = 0, *otherArrayMember = 0;
    QString separator;

    if (!inDefaultProperty) {
        QmlJS::AST::UiArrayBinding *initializer = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(parent);
        Q_ASSERT(initializer);

        otherArrayMember = 0;
        for (QmlJS::AST::UiArrayMemberList *cur = initializer->members; cur; cur = cur->next) {
            if (cur->member == movingObject) {
                arrayMember = cur;
                if (cur->next)
                    otherArrayMember = cur->next;
                break;
            }
            otherArrayMember = cur;
        }
        Q_ASSERT(arrayMember && otherArrayMember);
        separator = QStringLiteral(",");
    }

    moveInfo.objectStart = movingObject->firstSourceLocation().offset;
    moveInfo.objectEnd = movingObject->lastSourceLocation().end();

    int start = moveInfo.objectStart;
    int end = moveInfo.objectEnd;
    if (!inDefaultProperty) {
        if (arrayMember->commaToken.isValid())
            start = arrayMember->commaToken.begin();
        else
            end = otherArrayMember->commaToken.end();
    }

    includeSurroundingWhitespace(start, end);
    moveInfo.leadingCharsToRemove = moveInfo.objectStart - start;
    moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;

    if (beforeObject) {
        moveInfo.destination = beforeObject->firstSourceLocation().offset;
        int dummy = -1;
        includeSurroundingWhitespace(moveInfo.destination, dummy);

        moveInfo.prefixToInsert = QString(moveInfo.leadingCharsToRemove, QLatin1Char(' '));
        moveInfo.suffixToInsert = separator + QStringLiteral("\n\n");
    } else {
        const QmlJS::AST::SourceLocation insertionPoint = lastParentLocation();
        Q_ASSERT(insertionPoint.isValid());
        moveInfo.destination = insertionPoint.offset;
        int dummy = -1;
        includeSurroundingWhitespace(moveInfo.destination, dummy);

        moveInfo.prefixToInsert = separator + QString(moveInfo.leadingCharsToRemove, QLatin1Char(' '));
        moveInfo.suffixToInsert = QStringLiteral("\n");
    }

    move(moveInfo);
    setDidRewriting(true);
}

QmlJS::AST::Node *MoveObjectBeforeObjectVisitor::movingObjectParent() const
{
    if (movingObjectParents.size() > 1)
        return movingObjectParents.at(movingObjectParents.size() - 2);
    else
        return 0;
}

QmlJS::AST::SourceLocation MoveObjectBeforeObjectVisitor::lastParentLocation() const
{
    dump(movingObjectParents);

    QmlJS::AST::Node *parent = movingObjectParent();
    if (QmlJS::AST::UiObjectInitializer *initializer = QmlJS::AST::cast<QmlJS::AST::UiObjectInitializer*>(parent))
        return initializer->rbraceToken;
    else if (QmlJS::AST::UiArrayBinding *initializer = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(parent))
        return initializer->rbracketToken;
    else
        return QmlJS::AST::SourceLocation();
}
