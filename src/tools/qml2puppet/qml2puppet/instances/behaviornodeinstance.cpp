// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviornodeinstance.h"

#include <qmlprivategate.h>

namespace QmlDesigner {
namespace Internal {

BehaviorNodeInstance::BehaviorNodeInstance(QObject *object)
    : ObjectNodeInstance(object),
    m_isEnabled(true)
{
}

BehaviorNodeInstance::Pointer BehaviorNodeInstance::create(QObject *object)
{
    Pointer instance(new BehaviorNodeInstance(object));

    instance->populateResetHashes();

    QmlPrivateGate::disableBehaivour(object);

    return instance;
}

QVariant BehaviorNodeInstance::property(const PropertyName &name) const
{
    if (name == "enabled")
        return QVariant::fromValue(m_isEnabled);

    return ObjectNodeInstance::property(name);
}

void BehaviorNodeInstance::resetProperty(const PropertyName &name)
{
    if (name == "enabled")
        m_isEnabled = true;

    ObjectNodeInstance::resetProperty(name);
}

PropertyNameList BehaviorNodeInstance::ignoredProperties() const
{
    return PropertyNameList({"enabled"});
}


} // namespace Internal
} // namespace QmlDesigner
