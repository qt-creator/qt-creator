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

#include "idwarningitem.h"
#include <QGraphicsScene>

using namespace ScxmlEditor::PluginInterface;

IdWarningItem::IdWarningItem(QGraphicsItem *parent)
    : WarningItem(parent)
{
    setSeverity(OutputPane::Warning::ErrorType);
    setTypeName(tr("State"));
    setDescription(tr("Each State has to be unique ID."));
    setReason(tr("Missing ID"));
    setX(-boundingRect().width());
}

void IdWarningItem::check()
{
    setId(m_id);
}

void IdWarningItem::setId(const QString &text)
{
    QString oldId = m_id;
    m_id = text;

    // Check old id
    if (!oldId.isEmpty())
        checkDuplicates(oldId);

    // Check new id
    if (m_id.isEmpty()) {
        setReason(tr("Missing ID"));
        setWarningActive(true);
    } else
        checkDuplicates(m_id);
}

void IdWarningItem::checkDuplicates(const QString &id)
{
    if (scene()) {
        QVector<IdWarningItem*> foundItems;

        QList<QGraphicsItem*> items = scene()->items();
        for (int i = 0; i < items.count(); ++i) {
            if (items[i]->type() == IdWarningType) {
                auto item = qgraphicsitem_cast<IdWarningItem*>(items[i]);
                if (item && item->id() == id)
                    foundItems << item;
            }
        }

        if (foundItems.count() == 1) {
            foundItems[0]->setWarningActive(false);
        } else {
            for (int i = 0; i < foundItems.count(); ++i) {
                foundItems[i]->setReason(tr("Duplicate ID (%1)").arg(id));
                foundItems[i]->setWarningActive(true);
            }
        }
    }
}
