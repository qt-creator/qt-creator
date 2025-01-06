// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class BehaviorNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<BehaviorNodeInstance>;
    using WeakPointer = QWeakPointer<BehaviorNodeInstance>;

    BehaviorNodeInstance(QObject *object);

    static Pointer create(QObject *objectToBeWrapped);

    QVariant property(const PropertyName &name) const override;
    void resetProperty(const PropertyName &name) override;

    PropertyNameList ignoredProperties() const override;

private:
    bool m_isEnabled;
};

} // namespace Internal
} // namespace QmlDesigner
