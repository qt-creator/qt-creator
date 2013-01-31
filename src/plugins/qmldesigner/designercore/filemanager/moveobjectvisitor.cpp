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

#include "moveobjectvisitor.h"
#include "textmodifier.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlDesigner::Internal;
using namespace QmlDesigner;

class Inserter: public QMLRewriter
{
public:
    Inserter(QmlDesigner::TextModifier &modifier,
             quint32 targetParentObjectLocation,
             const QString &targetPropertyName,
             bool targetIsArrayBinding,
             TextModifier::MoveInfo moveInfo,
             const QStringList &propertyOrder):
        QMLRewriter(modifier),
        targetParentObjectLocation(targetParentObjectLocation),
        targetPropertyName(targetPropertyName),
        targetIsArrayBinding(targetIsArrayBinding),
        moveInfo(moveInfo),
        propertyOrder(propertyOrder)
    {}

protected:
    virtual bool visit(UiObjectBinding *ast)
    {
        if (didRewriting())
            return false;

        if (ast->qualifiedTypeNameId->identifierToken.offset == targetParentObjectLocation)
            insertInto(ast->initializer);

        return !didRewriting();
    }

    virtual bool visit(UiObjectDefinition *ast)
    {
        if (didRewriting())
            return false;

        if (ast->firstSourceLocation().offset == targetParentObjectLocation)
            insertInto(ast->initializer);

        return !didRewriting();
    }

private:
    void insertInto(UiObjectInitializer *ast)
    {
        if (targetPropertyName.isEmpty()) {
            // insert as UiObjectDefinition:
            UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members, propertyOrder);

            if (insertAfter && insertAfter->member) {
                moveInfo.destination = insertAfter->member->lastSourceLocation().end();
                moveInfo.prefixToInsert = QLatin1String("\n\n");
            } else {
                moveInfo.destination = ast->lbraceToken.end();
                moveInfo.prefixToInsert = QLatin1String("\n");
            }

            move(moveInfo);

            setDidRewriting(true);
            return;
        }

        // see if we need to insert into an UiArrayBinding:
        for (UiObjectMemberList *iter = ast->members; iter; iter = iter->next) {
            UiObjectMember *member = iter->member;

            if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member)) {
                if (toString(arrayBinding->qualifiedId) == targetPropertyName) {
                    appendToArray(arrayBinding);

                    setDidRewriting(true);
                    return;
                }
            }
        }

        { // insert (create) a UiObjectBinding:
            UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members, targetPropertyName, propertyOrder);
            moveInfo.prefixToInsert = QLatin1String("\n") + targetPropertyName + (targetIsArrayBinding ? QLatin1String(": [") : QLatin1String(": "));
            moveInfo.suffixToInsert = targetIsArrayBinding ? QLatin1String("\n]") : QLatin1String("");

            if (insertAfter && insertAfter->member)
                moveInfo.destination = insertAfter->member->lastSourceLocation().end();
            else
                moveInfo.destination = ast->lbraceToken.end();

            move(moveInfo);

            setDidRewriting(true);
        }
    }

    void appendToArray(UiArrayBinding *ast)
    {
        UiObjectMember *lastMember = 0;

        for (UiArrayMemberList *iter = ast->members; iter; iter = iter->next) {
            if (iter->member)
                lastMember = iter->member;
        }

        if (!lastMember)
            Q_ASSERT(!"Invalid QML: empty array found.");

        moveInfo.destination = lastMember->lastSourceLocation().end();
        moveInfo.prefixToInsert = QLatin1String(",\n");
        moveInfo.suffixToInsert = QLatin1String("\n");
        move(moveInfo);
    }

private:
    quint32 targetParentObjectLocation;
    QString targetPropertyName;
    bool targetIsArrayBinding;
    TextModifier::MoveInfo moveInfo;
    QStringList propertyOrder;
};

MoveObjectVisitor::MoveObjectVisitor(QmlDesigner::TextModifier &modifier,
                                     quint32 objectLocation,
                                     const QString &targetPropertyName,
                                     bool targetIsArrayBinding,
                                     quint32 targetParentObjectLocation,
                                     const QStringList &propertyOrder):
    QMLRewriter(modifier),
    objectLocation(objectLocation),
    targetPropertyName(targetPropertyName),
    targetIsArrayBinding(targetIsArrayBinding),
    targetParentObjectLocation(targetParentObjectLocation),
    propertyOrder(propertyOrder)
{
}

bool MoveObjectVisitor::operator()(UiProgram *ast)
{
    program = ast;

    return QMLRewriter::operator()(ast);
}

bool MoveObjectVisitor::visit(UiArrayBinding *ast)
{
    if (didRewriting())
        return false;

    UiArrayMemberList *currentMember = 0;

    for (UiArrayMemberList *it = ast->members; it; it = it->next) {
        if (it->member->firstSourceLocation().offset == objectLocation) {
            currentMember = it;
            break;
        }
    }

    if (currentMember) {
        TextModifier::MoveInfo moveInfo;
        moveInfo.objectStart = currentMember->member->firstSourceLocation().begin();
        moveInfo.objectEnd = currentMember->member->lastSourceLocation().end();

        if (currentMember == ast->members && !currentMember->next) {
            // array with 1 element, which is moved away, so also remove the array binding
            int start = ast->firstSourceLocation().offset;
            int end = ast->lastSourceLocation().end();
            includeSurroundingWhitespace(start, end);
            moveInfo.leadingCharsToRemove = objectLocation - start;
            moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;
        } else if (currentMember->next) {
            // we're not the last element, so remove the trailing comma too
            Q_ASSERT(currentMember->next->commaToken.isValid());

            int start = objectLocation;
            int end = currentMember->next->commaToken.end();
            includeSurroundingWhitespace(start, end);
            moveInfo.leadingCharsToRemove = objectLocation - start;
            moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;
        } else {
            // we are the last member, but not the only member, so remove the preceding comma, too
            Q_ASSERT(currentMember->commaToken.isValid());

            int start = currentMember->commaToken.offset;
            int end = moveInfo.objectEnd;
            includeSurroundingWhitespace(start, end);
            moveInfo.leadingCharsToRemove = objectLocation - start;
            moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;
        }

        doMove(moveInfo);
    }

    return !didRewriting();
}

bool MoveObjectVisitor::visit(UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == objectLocation) {
        TextModifier::MoveInfo moveInfo;
        moveInfo.objectStart = objectLocation;
        moveInfo.objectEnd = ast->lastSourceLocation().end();

        // remove leading indentation and property name:
        int start = ast->firstSourceLocation().offset;
        int end = moveInfo.objectEnd;
        includeSurroundingWhitespace(start, end);
        includeLeadingEmptyLine(start);
        moveInfo.leadingCharsToRemove = objectLocation - start;

        // remove trailing indentation
        moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;

        doMove(moveInfo);
    }

    return !didRewriting();
}

bool MoveObjectVisitor::visit(UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == objectLocation) {
        TextModifier::MoveInfo moveInfo;
        moveInfo.objectStart = objectLocation;
        moveInfo.objectEnd = ast->lastSourceLocation().end();

        // remove leading indentation:
        int start = objectLocation;
        int end = moveInfo.objectEnd;
        includeSurroundingWhitespace(start, end);
        includeLeadingEmptyLine(start);
        moveInfo.leadingCharsToRemove = objectLocation - start;
        moveInfo.trailingCharsToRemove = end - moveInfo.objectEnd;

        doMove(moveInfo);
    }

    return !didRewriting();
}

void MoveObjectVisitor::doMove(TextModifier::MoveInfo moveInfo)
{
    if (moveInfo.objectEnd > moveInfo.objectStart) {
        Inserter findTargetAndInsert(*textModifier(),
                                     targetParentObjectLocation,
                                     targetPropertyName,
                                     targetIsArrayBinding,
                                     moveInfo,
                                     propertyOrder);
        setDidRewriting(findTargetAndInsert(program));
    }
}
