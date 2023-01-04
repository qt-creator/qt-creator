// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutnodeinstance.h"

#include <QCoreApplication>

namespace QmlDesigner {
namespace Internal {

LayoutNodeInstance::LayoutNodeInstance(QQuickItem *item)
    : QuickItemNodeInstance(item)
{
}

bool LayoutNodeInstance::isLayoutable() const
{
    return true;
}

bool LayoutNodeInstance::isResizable() const
{
    return true;
}

LayoutNodeInstance::Pointer LayoutNodeInstance::create(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem*>(object);

    Q_ASSERT(item);

    Pointer instance(new LayoutNodeInstance(item));

    instance->setHasContent(anyItemHasContent(item));
    item->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(item)->classBegin();

    instance->populateResetHashes();

    return instance;
}

void LayoutNodeInstance::refreshLayoutable()
{
    if (quickItem()->parent())
        QCoreApplication::postEvent(quickItem(), new QEvent(QEvent::LayoutRequest));

}

PropertyNameList LayoutNodeInstance::ignoredProperties() const
{
    static const PropertyNameList properties({"move", "add", "populate"});
    return properties;
}

}
} // namespace QmlDesigner
