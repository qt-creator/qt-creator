// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "abstractproperty.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT SignalHandlerProperty final : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

public:
    void setSource(const QString &source);
    QString source() const;

    SignalHandlerProperty();
    SignalHandlerProperty(const SignalHandlerProperty &property, AbstractView *view);

    static PropertyName prefixAdded(PropertyNameView propertyName);
    static PropertyName prefixRemoved(PropertyNameView propertyName);

    SignalHandlerProperty(PropertyNameView propertyName,
                          const Internal::InternalNodePointer &internalNode,
                          Model *model,
                          AbstractView *view)
        : AbstractProperty(propertyName, internalNode, model, view)
    {}
};

class QMLDESIGNERCORE_EXPORT SignalDeclarationProperty final : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

public:
    void setSignature(const QString &source);
    QString signature() const;

    SignalDeclarationProperty();
    SignalDeclarationProperty(const SignalDeclarationProperty &property, AbstractView *view);

    SignalDeclarationProperty(PropertyNameView propertyName,
                              const Internal::InternalNodePointer &internalNode,
                              Model *model,
                              AbstractView *view)
        : AbstractProperty(propertyName, internalNode, model, view)
    {}
};

} // namespace QmlDesigner
