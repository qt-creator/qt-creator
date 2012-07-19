/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "anchorlineindicator.h"

#include <QSet>

namespace QmlDesigner {

AnchorLineIndicator::AnchorLineIndicator(LayerItem *layerItem)
  :  m_layerItem(layerItem)
{
    Q_ASSERT(layerItem);
}

AnchorLineIndicator::~AnchorLineIndicator()
{
    m_itemControllerHash.clear();
}

void AnchorLineIndicator::show(AnchorLine::Type anchorLineMask)
{
    QHashIterator<FormEditorItem*, AnchorLineController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        AnchorLineController controller = itemControllerIterator.next().value();
        controller.show(anchorLineMask);
    }
}

void AnchorLineIndicator::hide()
{
    QHashIterator<FormEditorItem*, AnchorLineController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        AnchorLineController controller = itemControllerIterator.next().value();
        controller.hide();
    }
}

void AnchorLineIndicator::clear()
{
    m_itemControllerHash.clear();
}

void AnchorLineIndicator::setItem(FormEditorItem* item)
{
    if (!item)
        return;

    QList<FormEditorItem*> itemList;
    itemList.append(item);

    setItems(itemList);
}

static bool equalLists(const QList<FormEditorItem*> &firstList, const QList<FormEditorItem*> &secondList)
{
    return firstList.toSet() == secondList.toSet();
}

void AnchorLineIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    if (equalLists(itemList, m_itemControllerHash.keys()))
        return;

    clear();

    foreach (FormEditorItem *item, itemList) {
        AnchorLineController controller(m_layerItem, item);
        m_itemControllerHash.insert(item, controller);
    }

    show(AnchorLine::AllMask);
}

void AnchorLineIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (m_itemControllerHash.contains(item)) {
            AnchorLineController controller(m_itemControllerHash.value(item));
            controller.updatePosition();
        }
    }
}

void AnchorLineIndicator::update()
{
    foreach (FormEditorItem *item, m_itemControllerHash.keys()) {
        if (m_itemControllerHash.contains(item)) {
            AnchorLineController controller(m_itemControllerHash.value(item));
            controller.updatePosition();
        }
    }
}

void AnchorLineIndicator::clearHighlight()
{
    foreach (FormEditorItem *item, m_itemControllerHash.keys()) {
        if (m_itemControllerHash.contains(item)) {
            AnchorLineController controller(m_itemControllerHash.value(item));
            controller.clearHighlight();
        }
    }
}

} // namespace QmlDesigner
