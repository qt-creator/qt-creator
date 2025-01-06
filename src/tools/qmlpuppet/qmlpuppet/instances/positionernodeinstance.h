// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "quickitemnodeinstance.h"

QT_BEGIN_NAMESPACE
class QQuickBasePositioner;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class PositionerNodeInstance : public QuickItemNodeInstance
{
public:
    using Pointer = QSharedPointer<PositionerNodeInstance>;
    using WeakPointer = QWeakPointer<PositionerNodeInstance>;

    static Pointer create(QObject *objectToBeWrapped);

    bool isPositioner() const override;
    bool isLayoutable() const override;

    bool isResizable() const override;

    void refreshLayoutable() override;

    PropertyNameList ignoredProperties() const override;

protected:
    PositionerNodeInstance(QQuickItem *item);
};

} // namespace Internal
} // namespace QmlDesigner
