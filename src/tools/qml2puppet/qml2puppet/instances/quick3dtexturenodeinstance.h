// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class Quick3DTextureNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<Quick3DTextureNodeInstance>;
    using WeakPointer = QWeakPointer<Quick3DTextureNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    void resetProperty(const PropertyName &name) override;

protected:
    Quick3DTextureNodeInstance(QObject *item);

private:
    void handleRedrawTimeout();

    bool m_multiPassNeeded = false;
};

} // namespace Internal
} // namespace QmlDesigner
