// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resizeindicator.h"

#include "formeditoritem.h"

namespace QmlDesigner {

ResizeIndicator::ResizeIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
    Q_ASSERT(layerItem);
}

ResizeIndicator::~ResizeIndicator()
{
    m_itemControllerHash.clear();
}

void ResizeIndicator::show()
{
    for (ResizeController controller : std::as_const(m_itemControllerHash))
        controller.show();
}

void ResizeIndicator::hide()
{
    for (ResizeController controller : std::as_const(m_itemControllerHash))
        controller.hide();
}

void ResizeIndicator::clear()
{
    m_itemControllerHash.clear();
}

static bool itemIsResizable(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.isValid()
            && qmlItemNode.instanceIsResizable()
            && qmlItemNode.modelIsMovable()
            && qmlItemNode.modelIsResizable()
            && !qmlItemNode.instanceHasScaleOrRotationTransform()
            && !qmlItemNode.instanceIsInLayoutable();
}

void ResizeIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    for (FormEditorItem* item : itemList) {
        if (item && itemIsResizable(item->qmlItemNode())) {
            ResizeController controller(m_layerItem, item);
            m_itemControllerHash.insert(item, controller);
        }
    }
}

void ResizeIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    for (FormEditorItem* item : itemList) {
        if (m_itemControllerHash.contains(item)) {
            if (!item || !itemIsResizable(item->qmlItemNode())) {
                m_itemControllerHash.take(item);
            } else {
                ResizeController controller(m_itemControllerHash.value(item));
                controller.updatePosition();
            }
        } else if (item && itemIsResizable(item->qmlItemNode())) {
            ResizeController controller(m_layerItem, item);
            m_itemControllerHash.insert(item, controller);
        }
    }
}

}
