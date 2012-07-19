/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef MOVEOBJECTBEFOREOBJECTVISITOR_H
#define MOVEOBJECTBEFOREOBJECTVISITOR_H

#include "qmlrewriter.h"

#include <QStack>

namespace QmlDesigner {
namespace Internal {

class MoveObjectBeforeObjectVisitor: public QMLRewriter
{
public:
    MoveObjectBeforeObjectVisitor(QmlDesigner::TextModifier &modifier,
                                  quint32 movingObjectLocation,
                                  bool inDefaultProperty);
    MoveObjectBeforeObjectVisitor(QmlDesigner::TextModifier &modifier,
                                  quint32 movingObjectLocation,
                                  quint32 beforeObjectLocation,
                                  bool inDefaultProperty);

    bool operator ()(QmlJS::AST::UiProgram *ast);

protected:
    virtual bool preVisit(QmlJS::AST::Node *ast);
    virtual void postVisit(QmlJS::AST::Node *ast);

    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    bool foundEverything() const
    { return movingObject != 0 && !movingObjectParents.isEmpty() && (toEnd || beforeObject != 0); }

    void doMove();

    QmlJS::AST::Node *movingObjectParent() const;
    QmlJS::AST::SourceLocation lastParentLocation() const;

private:
    QStack<QmlJS::AST::Node *> parents;
    quint32 movingObjectLocation;
    bool inDefaultProperty;
    bool toEnd;
    quint32 beforeObjectLocation;

    QmlJS::AST::UiObjectDefinition *movingObject;
    QmlJS::AST::UiObjectDefinition *beforeObject;
    ASTPath movingObjectParents;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // MOVEOBJECTBEFOREOBJECTVISITOR_H
