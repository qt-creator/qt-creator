// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

#include <QPair>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QQmlProperty;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class AnchorChangesNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<AnchorChangesNodeInstance>;
    using WeakPointer = QWeakPointer<AnchorChangesNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    QVariant property(const PropertyName &name) const override;
    void resetProperty(const PropertyName &name) override;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance,
                  const PropertyName &oldParentProperty,
                  const ObjectNodeInstance::Pointer &newParentInstance,
                  const PropertyName &newParentProperty) override;

protected:
    AnchorChangesNodeInstance(QObject *object);
    QObject *changesObject() const;
};

} // namespace Internal
} // namespace QmlDesigner
