// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class Quick3DReflectionProbeNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<Quick3DReflectionProbeNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);
    virtual ~Quick3DReflectionProbeNodeInstance();

    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void resetProperty(const PropertyName &name) override;

protected:
    Quick3DReflectionProbeNodeInstance(QObject *item);

private:
    void requestExtraUpdates();
    bool m_multiPassNeeded = false;
    bool m_needIncrement = true;
};

} // namespace Internal
} // namespace QmlDesigner
