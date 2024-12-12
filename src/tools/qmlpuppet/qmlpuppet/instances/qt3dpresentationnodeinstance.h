// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class Qt3DPresentationNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<Qt3DPresentationNodeInstance>;
    using WeakPointer = QWeakPointer<Qt3DPresentationNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    PropertyNameList ignoredProperties() const override;

protected:
    Qt3DPresentationNodeInstance(QObject *item);
};

} // namespace Internal
} // namespace QmlDesigner
