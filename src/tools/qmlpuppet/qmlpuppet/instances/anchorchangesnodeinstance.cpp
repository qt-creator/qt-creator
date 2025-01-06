// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "anchorchangesnodeinstance.h"

namespace QmlDesigner {

namespace Internal {

AnchorChangesNodeInstance::AnchorChangesNodeInstance(QObject *object) :
        ObjectNodeInstance(object)
{
}

AnchorChangesNodeInstance::Pointer AnchorChangesNodeInstance::create(QObject *object)
{
    Q_ASSERT(object);

    Pointer instance(new AnchorChangesNodeInstance(object));

    return instance;
}

void AnchorChangesNodeInstance::setPropertyVariant(const PropertyName &/*name*/, const QVariant &/*value*/)
{
}

void AnchorChangesNodeInstance::setPropertyBinding(const PropertyName &/*name*/, const QString &/*expression*/)
{
}

QVariant AnchorChangesNodeInstance::property(const PropertyName &/*name*/) const
{
    return QVariant();
}

void AnchorChangesNodeInstance::resetProperty(const PropertyName &/*name*/)
{
}

void AnchorChangesNodeInstance::reparent(const ObjectNodeInstance::Pointer &/*oldParentInstance*/,
                                         const PropertyName &/*oldParentProperty*/,
                                         const ObjectNodeInstance::Pointer &/*newParentInstance*/,
                                         const PropertyName &/*newParentProperty*/)
{
}

QObject *AnchorChangesNodeInstance::changesObject() const
{
    return object();
}

} // namespace Internal

} // namespace QmlDesigner
