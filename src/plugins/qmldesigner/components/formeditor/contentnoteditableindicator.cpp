// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentnoteditableindicator.h"
#include "nodemetainfo.h"

#include <utils/algorithm.h>

#include <QSet>
#include <QPen>

namespace QmlDesigner {

ContentNotEditableIndicator::ContentNotEditableIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{

}

ContentNotEditableIndicator::ContentNotEditableIndicator() = default;

ContentNotEditableIndicator::~ContentNotEditableIndicator()
{
    clear();
}

void ContentNotEditableIndicator::clear()
{
    for (const EntryPair &entryPair : std::as_const(m_entryList)) {
        delete entryPair.second;
        entryPair.first->blurContent(false);
    }

    m_entryList.clear();
}

bool operator ==(const ContentNotEditableIndicator::EntryPair &firstPair, const ContentNotEditableIndicator::EntryPair &secondPair)
{
    return firstPair.first == secondPair.first;
}

void ContentNotEditableIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    removeEntriesWhichAreNotInTheList(itemList);
    addAddiationEntries(itemList);
}

void ContentNotEditableIndicator::updateItems(const QList<FormEditorItem *> &itemList)
{
    QSet<FormEditorItem*> affectedFormEditorItemItems;
    affectedFormEditorItemItems.unite(Utils::toSet(itemList));
    for (FormEditorItem *formEditorItem : itemList)
        affectedFormEditorItemItems.unite(Utils::toSet(formEditorItem->offspringFormEditorItems()));

    for (const EntryPair &entryPair : std::as_const(m_entryList)) {
        for (FormEditorItem *formEditorItem : std::as_const(affectedFormEditorItemItems)) {
            if (formEditorItem == entryPair.first) {
                QRectF boundingRectangleInSceneSpace
                    = formEditorItem->qmlItemNode().instanceSceneTransform().mapRect(
                        formEditorItem->qmlItemNode().instanceBoundingRect());
                entryPair.second->setRect(boundingRectangleInSceneSpace);
                entryPair.second->update();
            }
        }
    }
}

void ContentNotEditableIndicator::addAddiationEntries(const QList<FormEditorItem *> &itemList)
{
    for (FormEditorItem *formEditorItem : itemList) {
        const ModelNode modelNode = formEditorItem->qmlItemNode().modelNode();
        if (modelNode.metaInfo().isValid() && modelNode.metaInfo().isQtQuickLoader()) {
            if (!m_entryList.contains(EntryPair(formEditorItem, 0))) {
                auto indicatorShape = new QGraphicsRectItem(m_layerItem);
                QPen linePen;
                linePen.setCosmetic(true);
                linePen.setColor(QColor(0xa0, 0xa0, 0xa0));
                indicatorShape->setPen(linePen);
                QRectF boundingRectangleInSceneSpace = formEditorItem->qmlItemNode().instanceSceneTransform().mapRect(formEditorItem->qmlItemNode().instanceBoundingRect());
                indicatorShape->setRect(boundingRectangleInSceneSpace);
                static QBrush brush(QColor(0, 0, 0, 10), Qt::BDiagPattern);
                indicatorShape->setBrush(brush);

                m_entryList.append(EntryPair(formEditorItem, indicatorShape));
            }
        }
    }
}

void ContentNotEditableIndicator::removeEntriesWhichAreNotInTheList(const QList<FormEditorItem *> &itemList)
{
    for (int i = 0; i < m_entryList.size(); ++i) {
        const EntryPair &entryPair = m_entryList.at(i);
        if (!itemList.contains(entryPair.first)) {
            delete entryPair.second;
            entryPair.first->blurContent(false);
            m_entryList.removeAt(i--);
        }
    }
}

} // namespace QmlDesigner
