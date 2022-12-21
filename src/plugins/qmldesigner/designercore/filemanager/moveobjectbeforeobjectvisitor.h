// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    bool operator ()(QmlJS::AST::UiProgram *ast) override;

protected:
    bool preVisit(QmlJS::AST::Node *ast) override;
    void postVisit(QmlJS::AST::Node *ast) override;

    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

private:
    bool foundEverything() const
    { return movingObject != nullptr && !movingObjectParents.isEmpty() && (toEnd || beforeObject != nullptr); }

    void doMove();

    QmlJS::AST::Node *movingObjectParent() const;
    QmlJS::SourceLocation lastParentLocation() const;

private:
    QStack<QmlJS::AST::Node *> parents;
    quint32 movingObjectLocation;
    bool inDefaultProperty;
    bool toEnd;
    quint32 beforeObjectLocation;

    QmlJS::AST::UiObjectDefinition *movingObject = nullptr;
    QmlJS::AST::UiObjectDefinition *beforeObject = nullptr;
    ASTPath movingObjectParents;
};

} // namespace Internal
} // namespace QmlDesigner
