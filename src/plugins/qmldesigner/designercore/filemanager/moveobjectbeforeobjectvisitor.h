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
