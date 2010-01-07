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
    QHashIterator<FormEditorItem*, ResizeController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        ResizeController controller = itemControllerIterator.next().value();
        controller.show();
    }
}
void ResizeIndicator::hide()
{
    QHashIterator<FormEditorItem*, ResizeController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        ResizeController controller = itemControllerIterator.next().value();
        controller.hide();
    }
}

void ResizeIndicator::clear()
{
    m_itemControllerHash.clear();
}

void ResizeIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    foreach (FormEditorItem* item, itemList) {
        if (item->qmlItemNode().isRootNode())
            continue;
        ResizeController controller(m_layerItem, item);
        m_itemControllerHash.insert(item, controller);
    }
}

void ResizeIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem* item, itemList) {
        if (m_itemControllerHash.contains(item)) {
            ResizeController controller(m_itemControllerHash.value(item));
            controller.updatePosition();
        }
    }
}

}
