// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "quickitemnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class LayoutNodeInstance : public QuickItemNodeInstance
{

public:
    using Pointer = QSharedPointer<LayoutNodeInstance>;
    using WeakPointer = QWeakPointer<LayoutNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    bool isLayoutable() const override;

    bool isResizable() const override;

    void refreshLayoutable() override;

    PropertyNameList ignoredProperties() const override;

protected:
    LayoutNodeInstance(QQuickItem *item);
};

} // namespace Internal
} // namespace QmlDesigner
