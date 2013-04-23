/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

void LayoutNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    if (name == "move" || name == "add" || name == "populate")
        return;

    QuickItemNodeInstance::setPropertyVariant(name, value);
}

void LayoutNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (name == "move" || name == "add" || name == "populate")
        return;

    QuickItemNodeInstance::setPropertyBinding(name, expression);
}

LayoutNodeInstance::Pointer LayoutNodeInstance::create(QObject *object)
{
    qDebug() << "layout" << object;
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
    qDebug() << "before";
    if (quickItem()->parent())
        QCoreApplication::postEvent(quickItem(), new QEvent(QEvent::LayoutRequest));
    qDebug() << "refresh";

}

}
} // namespace QmlDesigner
