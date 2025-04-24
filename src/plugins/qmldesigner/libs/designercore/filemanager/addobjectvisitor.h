// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class AddObjectVisitor: public QMLRewriter
{
public:
    AddObjectVisitor(QmlDesigner::TextModifier &modifier,
                     quint32 parentLocation,
                     std::optional<int> nodeLocation,
                     const QString &content,
                     Utils::span<const PropertyNameView> propertyOrder);

protected:
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

private:
    void insertInto(QmlJS::AST::UiObjectInitializer *ast);

private:
    quint32 m_parentLocation;
    std::optional<int> m_nodeLocation;
    QString m_content;
    Utils::span<const PropertyNameView> m_propertyOrder;
};

} // namespace Internal
} // namespace QmlDesigner
