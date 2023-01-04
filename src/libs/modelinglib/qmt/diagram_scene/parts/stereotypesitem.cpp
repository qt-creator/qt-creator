// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stereotypesitem.h"

namespace qmt {

StereotypesItem::StereotypesItem(QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(parent)
{
}

StereotypesItem::~StereotypesItem()
{
}

void StereotypesItem::setStereotypes(const QList<QString> &stereotypes)
{
    setText(format(stereotypes));
}

QString StereotypesItem::format(const QList<QString> &stereotypes)
{
    QString text;
    if (!stereotypes.isEmpty()) {
        text = QString::fromUtf8("«");
        bool first = true;
        for (const QString &stereotype : stereotypes) {
            if (!first)
                text += ", ";
            text += stereotype;
            first = false;
        }
        text += QString::fromUtf8("»");
    }
    return text;
}

} // namespace qmt
