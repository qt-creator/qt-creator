/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "rotationindicator.h"

#include "formeditoritem.h"

namespace QmlDesigner {

RotationIndicator::RotationIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
    Q_ASSERT(layerItem);
}

RotationIndicator::~RotationIndicator()
{
    m_itemControllerHash.clear();
}

void RotationIndicator::show()
{
    for (RotationController controller : qAsConst(m_itemControllerHash))
        controller.show();
}

void RotationIndicator::hide()
{
    for (RotationController controller : qAsConst(m_itemControllerHash))
        controller.hide();
}

void RotationIndicator::clear()
{
    m_itemControllerHash.clear();
}

static bool itemIsRotatable(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.isValid()
            && qmlItemNode.instanceIsResizable()
            && qmlItemNode.modelIsMovable()
            && qmlItemNode.modelIsRotatable()
            && !qmlItemNode.instanceIsInLayoutable()
            && !qmlItemNode.isFlowItem();
}

void RotationIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    for (FormEditorItem *item : itemList) {
        if (item && itemIsRotatable(item->qmlItemNode())) {
            RotationController controller(m_layerItem, item);
            m_itemControllerHash.insert(item, controller);
        }
    }
}

void RotationIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    for (FormEditorItem *item : itemList) {
        if (m_itemControllerHash.contains(item)) {
            if (!item || !itemIsRotatable(item->qmlItemNode())) {
                m_itemControllerHash.take(item);
            } else {
                RotationController controller(m_itemControllerHash.value(item));
                controller.updatePosition();
            }
        } else if (item && itemIsRotatable(item->qmlItemNode())) {
            RotationController controller(m_layerItem, item);
            m_itemControllerHash.insert(item, controller);
        }
    }
}

}
