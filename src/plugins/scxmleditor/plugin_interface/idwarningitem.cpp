// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idwarningitem.h"
#include "scxmleditortr.h"

#include <QGraphicsScene>

using namespace ScxmlEditor::PluginInterface;

IdWarningItem::IdWarningItem(QGraphicsItem *parent)
    : WarningItem(parent)
{
    setSeverity(OutputPane::Warning::ErrorType);
    setTypeName(Tr::tr("State"));
    setDescription(Tr::tr("Each state must have a unique ID."));
    setReason(Tr::tr("Missing ID."));
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
        setReason(Tr::tr("Missing ID."));
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
                foundItems[i]->setReason(Tr::tr("Duplicate ID (%1).").arg(id));
                foundItems[i]->setWarningActive(true);
            }
        }
    }
}
