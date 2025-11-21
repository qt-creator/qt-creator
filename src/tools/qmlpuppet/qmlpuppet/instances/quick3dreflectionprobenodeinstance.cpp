// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick3dreflectionprobenodeinstance.h"

namespace QmlDesigner {
namespace Internal {

Quick3DReflectionProbeNodeInstance::Quick3DReflectionProbeNodeInstance(QObject *object)
    : ObjectNodeInstance(object)
{
}

Quick3DReflectionProbeNodeInstance::Pointer Quick3DReflectionProbeNodeInstance::create(QObject *object)
{
    Pointer instance(new Quick3DReflectionProbeNodeInstance(object));

    instance->populateResetHashes();

    return instance;
}

Quick3DReflectionProbeNodeInstance::~Quick3DReflectionProbeNodeInstance()
{
    if (!m_needIncrement)
        nodeInstanceServer()->decrementReflectionProbes();
}

void Quick3DReflectionProbeNodeInstance::requestExtraUpdates()
{
    if (m_needIncrement) {
        // nodeInstanceServer pointer is set after construction, so we increment on first use
        m_needIncrement = false;
        nodeInstanceServer()->incrementReflectionProbes();
    }

    nodeInstanceServer()->requestExtra3dUpdates();
}

void Quick3DReflectionProbeNodeInstance::setPropertyBinding(const PropertyName &name,
                                                    const QString &expression)
{
    ObjectNodeInstance::setPropertyBinding(name, expression);

    requestExtraUpdates();
}

void Quick3DReflectionProbeNodeInstance::setPropertyVariant(const PropertyName &name,
                                                            const QVariant &value)
{
    ObjectNodeInstance::setPropertyVariant(name, value);

    requestExtraUpdates();
}

void Quick3DReflectionProbeNodeInstance::resetProperty(const PropertyName &name)
{
    ObjectNodeInstance::resetProperty(name);

    requestExtraUpdates();
}

} // namespace Internal
} // namespace QmlDesigner
