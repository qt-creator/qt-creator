// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlrefactoring.h"
#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class AddPropertyVisitor: public QMLRewriter
{
public:
public:
    AddPropertyVisitor(TextModifier &modifier,
                       quint32 parentLocation,
                       const PropertyName &name,
                       const QString &value,
                       QmlRefactoring::PropertyType propertyType,
                       const PropertyNameList &propertyOrder,
                       const TypeName &dynamicTypeName);

protected:
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;

private:
    void addInMembers(QmlJS::AST::UiObjectInitializer *initializer);

private:
    quint32 m_parentLocation;
    PropertyName m_name;
    QString m_value;
    QmlRefactoring::PropertyType m_propertyType;
    PropertyNameList m_propertyOrder;
    TypeName m_dynamicTypeName;
};

} // namespace Internal
} // namespace QmlDesigner
