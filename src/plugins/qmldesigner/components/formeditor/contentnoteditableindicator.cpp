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

#include "contentnoteditableindicator.h"
#include "nodemetainfo.h"

#include <QSet>
#include <QPen>

namespace QmlDesigner {

ContentNotEditableIndicator::ContentNotEditableIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{

}

ContentNotEditableIndicator::ContentNotEditableIndicator()
{
}

ContentNotEditableIndicator::~ContentNotEditableIndicator()
{
    clear();
}

void ContentNotEditableIndicator::clear()
{
    foreach (const EntryPair &entryPair, m_entryList) {
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
    affectedFormEditorItemItems.unite(itemList.toSet());
    foreach (FormEditorItem *formEditorItem, itemList)
        affectedFormEditorItemItems.unite(formEditorItem->offspringFormEditorItems().toSet());

    foreach (const EntryPair &entryPair, m_entryList) {
         foreach (FormEditorItem *formEditorItem, affectedFormEditorItemItems) {
             if (formEditorItem == entryPair.first) {
                 QRectF boundingRectangleInSceneSpace = formEditorItem->qmlItemNode().instanceSceneTransform().mapRect(formEditorItem->qmlItemNode().instanceBoundingRect());
                 entryPair.second->setRect(boundingRectangleInSceneSpace);
                 entryPair.second->update();
             }
         }
    }
}

void ContentNotEditableIndicator::addAddiationEntries(const QList<FormEditorItem *> &itemList)
{
    foreach (FormEditorItem *formEditorItem, itemList) {
        if (formEditorItem->qmlItemNode().modelNode().metaInfo().isSubclassOf("QtQuick.Loader")) {

            if (!m_entryList.contains(EntryPair(formEditorItem, 0))) {
                QGraphicsRectItem *indicatorShape = new QGraphicsRectItem(m_layerItem);
                QPen linePen;
                linePen.setCosmetic(true);
                linePen.setColor("#a0a0a0");
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
    QMutableListIterator<EntryPair> entryIterator(m_entryList);

    while (entryIterator.hasNext()) {
        EntryPair &entryPair = entryIterator.next();
        if (!itemList.contains(entryPair.first)) {
            delete entryPair.second;
            entryPair.first->blurContent(false);
            entryIterator.remove();
        }
    }
}

} // namespace QmlDesigner
