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

    using SL = ModelTracing::SourceLocation;

public:
    void setSource(const QString &source, SL sl = {});
    QString source(SL sl = {}) const;
    QString sourceNormalizedWithBraces(SL sl = {}) const;

    bool useNewFunctionSyntax(SL sl = {});

    SignalHandlerProperty();
    SignalHandlerProperty(const SignalHandlerProperty &property, AbstractView *view);

    static PropertyName prefixAdded(PropertyNameView propertyName, SL sl = {});
    static PropertyName prefixRemoved(PropertyNameView propertyName, SL sl = {});

    SignalHandlerProperty(PropertyNameView propertyName,
                          const Internal::InternalNodePointer &internalNode,
                          Model *model,
                          AbstractView *view)
        : AbstractProperty(propertyName, internalNode, model, view)
    {}

    static QString normalizedSourceWithBraces(const QString &source, SL sl = {});
};

class QMLDESIGNERCORE_EXPORT SignalDeclarationProperty final : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

    using SL = ModelTracing::SourceLocation;

public:
    void setSignature(const QString &source, SL sl = {});
    QString signature(SL sl = {}) const;

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
