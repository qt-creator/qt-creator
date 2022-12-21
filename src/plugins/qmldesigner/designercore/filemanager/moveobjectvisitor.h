// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class MoveObjectVisitor: public QMLRewriter
{
public:
    MoveObjectVisitor(QmlDesigner::TextModifier &modifier,
                      quint32 objectLocation,
                      const QmlDesigner::PropertyName &targetPropertyName,
                      bool targetIsArrayBinding,
                      quint32 targetParentObjectLocation,
                      const PropertyNameList &propertyOrder);

    bool operator ()(QmlJS::AST::UiProgram *ast) override;

protected:
    bool visit(QmlJS::AST::UiArrayBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

private:
    void doMove(const TextModifier::MoveInfo &moveInfo);

private:
    QList<QmlJS::AST::Node *> parents;
    quint32 objectLocation;
    PropertyName targetPropertyName;
    bool targetIsArrayBinding;
    quint32 targetParentObjectLocation;
    PropertyNameList propertyOrder;

    QmlJS::AST::UiProgram *program;
};

} // namespace Internal
} // namespace QmlDesigner
