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
