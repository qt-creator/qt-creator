/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "layoutnodeinstance.h"

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
    return PropertyNameList() << "move" << "add" << "populate";
}

}
} // namespace QmlDesigner
