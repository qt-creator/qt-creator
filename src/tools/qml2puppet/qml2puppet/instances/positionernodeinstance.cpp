// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "positionernodeinstance.h"
#include <QQuickItem>

namespace QmlDesigner {
namespace Internal {

PositionerNodeInstance::PositionerNodeInstance(QQuickItem *item)
    : QuickItemNodeInstance(item)
{
}

bool PositionerNodeInstance::isPositioner() const
{
    return true;
}

bool PositionerNodeInstance::isLayoutable() const
{
    return true;
}

bool PositionerNodeInstance::isResizable() const
{
    return true;
}

PositionerNodeInstance::Pointer PositionerNodeInstance::create(QObject *object)
{
    QQuickItem *positioner = qobject_cast<QQuickItem*>(object);

    Q_ASSERT(positioner);

    Pointer instance(new PositionerNodeInstance(positioner));

    instance->setHasContent(anyItemHasContent(positioner));
    positioner->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(positioner)->classBegin();

    instance->populateResetHashes();

    return instance;
}

void PositionerNodeInstance::refreshLayoutable()
{
    [[maybe_unused]] bool success = QMetaObject::invokeMethod(object(), "prePositioning");
    Q_ASSERT(success);
}

PropertyNameList PositionerNodeInstance::ignoredProperties() const
{
    static const PropertyNameList properties({"move", "add", "populate"});
    return properties;
}
} // namespace Internal
} // namespace QmlDesigner
