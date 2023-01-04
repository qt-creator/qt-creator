// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dummynodeinstance.h"

namespace QmlDesigner {
namespace Internal {

DummyNodeInstance::DummyNodeInstance()
   : ObjectNodeInstance(new QObject)
{
}

DummyNodeInstance::Pointer DummyNodeInstance::create()
{
    return Pointer(new DummyNodeInstance);
}

QRectF DummyNodeInstance::boundingRect() const
{
    return QRectF();
}

QPointF DummyNodeInstance::position() const
{
    return QPointF();
}

QSizeF DummyNodeInstance::size() const
{
    return QSizeF();
}

QTransform DummyNodeInstance::transform() const
{
    return QTransform();
}

double DummyNodeInstance::opacity() const
{
    return 0.0;
}

void DummyNodeInstance::setPropertyVariant(const PropertyName &/*name*/, const QVariant &/*value*/)
{
}

void DummyNodeInstance::setPropertyBinding(const PropertyName &/*name*/, const QString &/*expression*/)
{

}

void DummyNodeInstance::setId(const QString &/*id*/)
{

}

QVariant DummyNodeInstance::property(const PropertyName &/*name*/) const
{
    return QVariant();
}

void DummyNodeInstance::initializePropertyWatcher(const ObjectNodeInstance::Pointer &/*objectNodeInstance*/)
{

}

} // namespace Internal
} // namespace QmlDesigner
