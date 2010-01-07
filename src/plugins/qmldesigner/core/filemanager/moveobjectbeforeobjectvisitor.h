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

#ifndef MOVEOBJECTBEFOREOBJECTVISITOR_H
#define MOVEOBJECTBEFOREOBJECTVISITOR_H

#include "qmlrewriter.h"

#include <QtCore/QStack>

namespace QmlEditor {
namespace Internal {

class MoveObjectBeforeObjectVisitor: public QMLRewriter
{
public:
    MoveObjectBeforeObjectVisitor(QmlDesigner::TextModifier &modifier,
                                  quint32 movingObjectLocation);
    MoveObjectBeforeObjectVisitor(QmlDesigner::TextModifier &modifier,
                                  quint32 movingObjectLocation,
                                  quint32 beforeObjectLocation);

    bool operator ()(QmlJS::AST::UiProgram *ast);

protected:
    virtual bool preVisit(QmlJS::AST::Node *ast);
    virtual void postVisit(QmlJS::AST::Node *ast);

    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    bool foundEverything() const
    { return movingObject != 0 && !movingObjectParents.isEmpty() && (toEnd || beforeObject != 0); }

    void doMove();

    QmlJS::AST::SourceLocation lastParentLocation() const;

private:
    QStack<QmlJS::AST::Node *> parents;
    quint32 movingObjectLocation;
    bool toEnd;
    quint32 beforeObjectLocation;

    QmlJS::AST::UiObjectDefinition *movingObject;
    QmlJS::AST::UiObjectDefinition *beforeObject;
    ASTPath movingObjectParents;
};

} // namespace Internal
} // namespace QmlEditor

#endif // MOVEOBJECTBEFOREOBJECTVISITOR_H
