// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {

namespace Internal {

class QmlStateNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<QmlStateNodeInstance>;
    using WeakPointer = QWeakPointer<QmlStateNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;

    void activateState() override;
    void deactivateState() override;

    bool updateStateVariant(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &value) override;
    bool updateStateBinding(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QString &expression) override;
    bool resetStateProperty(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &resetValue) override;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance,
                  const PropertyName &oldParentProperty,
                  const ObjectNodeInstance::Pointer &newParentInstance,
                  const PropertyName &newParentProperty) override;

protected:
    QmlStateNodeInstance(QObject *object);
};

} // namespace Internal
} // namespace QmlDesigner
