// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rotationindicator.h"

#include "formeditoritem.h"

#include <designermcumanager.h>
#include <nodehints.h>
#include <nodemetainfo.h>

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
    for (RotationController controller : std::as_const(m_itemControllerHash))
        controller.show();
}

void RotationIndicator::hide()
{
    for (RotationController controller : std::as_const(m_itemControllerHash))
        controller.hide();
}

void RotationIndicator::clear()
{
    m_itemControllerHash.clear();
}
namespace {

bool itemIsResizable(const ModelNode &modelNode)
{
    return NodeHints::fromModelNode(modelNode).isResizable();
}

bool isMcuRotationAllowed([[maybe_unused]] QString itemName, [[maybe_unused]] bool hasChildren)
{
    const QString propName = "rotation";
    const DesignerMcuManager &manager = DesignerMcuManager::instance();
    if (manager.isMCUProject()) {
        if (manager.allowedItemProperties().contains(itemName)) {
            const DesignerMcuManager::ItemProperties properties = manager.allowedItemProperties().value(
                itemName);
            if (properties.properties.contains(propName)) {
                if (hasChildren)
                    return properties.allowChildren;
                return true;
            }
        }

        if (manager.bannedItems().contains(itemName))
            return false;

        if (manager.bannedProperties().contains(propName))
            return false;
    }

    return true;
}

bool modelIsRotatable(const QmlItemNode &itemNode)
{
    auto modelNode = itemNode.modelNode();
    return !modelNode.hasBindingProperty("rotation") && itemIsResizable(modelNode)
           && !itemNode.modelIsInLayout()
           && isMcuRotationAllowed(QString::fromUtf8(modelNode.type()), itemNode.hasChildren());
}

bool itemIsRotatable(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.isValid() && qmlItemNode.instanceIsResizable()
           && qmlItemNode.modelIsMovable() && modelIsRotatable(qmlItemNode)
           && !qmlItemNode.instanceIsInLayoutable() && !qmlItemNode.isFlowItem();
}

} // namespace

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

} // namespace QmlDesigner
