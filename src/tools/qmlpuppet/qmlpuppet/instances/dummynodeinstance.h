// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWeakPointer>

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class DummyNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<DummyNodeInstance>;
    using WeakPointer = QWeakPointer<DummyNodeInstance>;

    static Pointer create();

    QRectF boundingRect() const override;
    QPointF position() const override;
    QSizeF size() const override;
    QTransform transform() const override;
    double opacity() const override;

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    void setId(const QString &id) override;
    QVariant property(const PropertyName &name) const override;

    void initializePropertyWatcher(const ObjectNodeInstance::Pointer &objectNodeInstance);

protected:
    DummyNodeInstance();

};

}
}
