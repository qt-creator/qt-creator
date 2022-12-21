// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStack>

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class RemoveUIObjectMemberVisitor: public QMLRewriter
{
public:
    RemoveUIObjectMemberVisitor(QmlDesigner::TextModifier &modifier,
                                quint32 objectLocation);

protected:
    bool preVisit(QmlJS::AST::Node *ast) override;
    void postVisit(QmlJS::AST::Node *) override;

    bool visit(QmlJS::AST::UiPublicMember *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::UiSourceElement *ast) override;
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiScriptBinding *ast) override;
    bool visit(QmlJS::AST::UiArrayBinding *ast) override;

private:
    bool visitObjectMember(QmlJS::AST::UiObjectMember *ast);

    QmlJS::AST::UiArrayBinding *containingArray() const;

    void extendToLeadingOrTrailingComma(QmlJS::AST::UiArrayBinding *parentArray,
                                        QmlJS::AST::UiObjectMember *ast,
                                        int &start,
                                        int &end) const;

private:
    quint32 objectLocation;
    QStack<QmlJS::AST::Node*> parents;
};

} // namespace Internal
} // namespace QmlDesigner
