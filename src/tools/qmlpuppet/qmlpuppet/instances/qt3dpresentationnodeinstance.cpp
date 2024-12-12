// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt3dpresentationnodeinstance.h"
#include <QQuickItem>

namespace QmlDesigner {
namespace Internal {

Qt3DPresentationNodeInstance::Qt3DPresentationNodeInstance(QObject *object)
    : ObjectNodeInstance(object)
{
}

Qt3DPresentationNodeInstance::Pointer Qt3DPresentationNodeInstance::create(QObject *object)
{
    Pointer instance(new Qt3DPresentationNodeInstance(object));
    instance->populateResetHashes();

    return instance;
}

PropertyNameList Qt3DPresentationNodeInstance::ignoredProperties() const
{
    static const PropertyNameList properties({"source"});
    return properties;
}
} // namespace Internal
} // namespace QmlDesigner
