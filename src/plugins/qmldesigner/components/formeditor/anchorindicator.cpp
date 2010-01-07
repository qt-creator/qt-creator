/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "anchorindicator.h"

#include <QSet>

namespace QmlDesigner {

AnchorIndicator::AnchorIndicator(LayerItem *layerItem)
  :  m_layerItem(layerItem)
{
    Q_ASSERT(layerItem);
}

AnchorIndicator::~AnchorIndicator()
{
    m_itemControllerHash.clear();
}

void AnchorIndicator::show()
{
    QHashIterator<FormEditorItem*, AnchorController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        AnchorController controller = itemControllerIterator.next().value();
        controller.show();
    }
}


void AnchorIndicator::hide()
{
    QHashIterator<FormEditorItem*, AnchorController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        AnchorController controller = itemControllerIterator.next().value();
        controller.hide();
    }
}

void AnchorIndicator::clear()
{
    m_itemControllerHash.clear();
}

void AnchorIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    foreach (FormEditorItem *item, itemList) {
        AnchorController controller(m_layerItem, item);
        m_itemControllerHash.insert(item, controller);
    }

    updateItems(itemList);
}

void AnchorIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (m_itemControllerHash.contains(item)) {
            AnchorController controller(m_itemControllerHash.value(item));
            controller.updatePosition();
        }
    }
}

void AnchorIndicator::updateTargetPoint(FormEditorItem *item, AnchorLine::Type anchorLine, const QPointF &targetPoint)
{
    AnchorController controller(m_itemControllerHash.value(item));
    controller.updateTargetPoint(anchorLine, targetPoint);
}

void AnchorIndicator::clearHighlight()
{
    QHashIterator<FormEditorItem*, AnchorController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        AnchorController controller = itemControllerIterator.next().value();
        controller.clearHighlight();
    }
}

void AnchorIndicator::highlight(FormEditorItem *item, AnchorLine::Type anchorLine)
{
    if (m_itemControllerHash.contains(item)) {
         AnchorController controller(m_itemControllerHash.value(item));
         controller.highlight(anchorLine);
     }
}

} // namespace QmlDesigner
