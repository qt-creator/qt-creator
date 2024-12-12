// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class QmlTransitionNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<QmlTransitionNodeInstance>;
    using WeakPointer = QWeakPointer<QmlTransitionNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    bool isTransition() const override;

    PropertyNameList ignoredProperties() const override;

private:
    QmlTransitionNodeInstance(QObject *transition);
};
}
}
