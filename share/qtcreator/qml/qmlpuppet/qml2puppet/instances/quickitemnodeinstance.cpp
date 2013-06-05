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

#include "quickitemnodeinstance.h"

#include "qt5nodeinstanceserver.h"

#include <QQmlExpression>
#include <QQuickView>
#include <cmath>

#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>

#include <QHash>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

QuickItemNodeInstance::QuickItemNodeInstance(QQuickItem *item)
   : GraphicalNodeInstance(item),
     m_isResizable(true),
     m_isMovable(true)
{
}

QuickItemNodeInstance::~QuickItemNodeInstance()
{
}

static bool isContentItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{

    return item->parentItem()
            && nodeInstanceServer->hasInstanceForObject(item->parentItem())
            && nodeInstanceServer->instanceForObject(item->parentItem()).internalInstance()->contentItem() == item;
}

static QTransform transformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    if (isContentItem(item, nodeInstanceServer))
        return QTransform();

    QTransform toParentTransform = DesignerSupport::parentTransform(item);
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {

        return transformForItem(item->parentItem(), nodeInstanceServer) * toParentTransform;
    }

    return toParentTransform;
}

QTransform QuickItemNodeInstance::transform() const
{
    return transformForItem(quickItem(), nodeInstanceServer());
}


QObject *QuickItemNodeInstance::parent() const
{
    if (!quickItem() || !quickItem()->parentItem())
        return 0;

    return quickItem()->parentItem();
}
bool QuickItemNodeInstance::isMovable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isMovable && quickItem() && quickItem()->parentItem();
}

void QuickItemNodeInstance::setMovable(bool movable)
{
    m_isMovable = movable;
}

QuickItemNodeInstance::Pointer QuickItemNodeInstance::create(QObject *object)
{
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);

    Q_ASSERT(quickItem);

    Pointer instance(new QuickItemNodeInstance(quickItem));

    instance->setHasContent(anyItemHasContent(quickItem));
    quickItem->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(quickItem)->classBegin();

    instance->populateResetHashes();

    return instance;
}

QQuickItem *QuickItemNodeInstance::contentItem() const
{
    return m_contentItem.data();
}

void QuickItemNodeInstance::doComponentComplete()
{
    GraphicalNodeInstance::doComponentComplete();

    QQmlProperty contentItemProperty(quickItem(), "contentItem", engine());
    if (contentItemProperty.isValid())
        m_contentItem = contentItemProperty.read().value<QQuickItem*>();
}

QRectF QuickItemNodeInstance::contentItemBoundingBox() const
{
    if (contentItem()) {
        QTransform contentItemTransform = DesignerSupport::parentTransform(contentItem());
        return contentItemTransform.mapRect(contentItem()->boundingRect());
    }

    return QRectF();
}

QTransform QuickItemNodeInstance::contentItemTransform() const
{
    return DesignerSupport::parentTransform(contentItem());
}


bool QuickItemNodeInstance::isQuickItem() const
{
    return true;
}

bool QuickItemNodeInstance::isResizable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isResizable && quickItem() && quickItem()->parentItem();
}

void QuickItemNodeInstance::setResizable(bool resizable)
{
    m_isResizable = resizable;
}

void QuickItemNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    if (oldParentInstance && oldParentInstance->isPositioner()) {
        setInLayoutable(false);
        setMovable(true);
    }

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (newParentInstance && newParentInstance->isPositioner()) {
        setInLayoutable(true);
        setMovable(false);
    }

    if (oldParentInstance && oldParentInstance->isPositioner() && !(newParentInstance && newParentInstance->isPositioner())) {
        if (!hasBindingForProperty("x"))
            setPropertyVariant("x", x());

        if (!hasBindingForProperty("y"))
            setPropertyVariant("y", y());
    }

    refresh();
    DesignerSupport::updateDirtyNode(quickItem());

    if (parentInstance() && isInLayoutable())
        parentInstance()->refreshLayoutable();
}

bool QuickItemNodeInstance::isAnchoredBySibling() const
{
    if (quickItem()->parentItem()) {
        foreach (QQuickItem *siblingItem, quickItem()->parentItem()->childItems()) { // search in siblings for a anchor to this item
            if (siblingItem) {
                if (DesignerSupport::isAnchoredTo(siblingItem, quickItem()))
                    return true;
            }
        }
    }

    return false;
}



QQuickItem *QuickItemNodeInstance::quickItem() const
{
    if (object() == 0)
        return 0;

    return static_cast<QQuickItem*>(object());
}

} // namespace Internal
} // namespace QmlDesigner

