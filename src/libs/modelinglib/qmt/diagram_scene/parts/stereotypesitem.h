// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsSimpleTextItem>
#include <QList>

namespace qmt {

class StereotypesItem : public QGraphicsSimpleTextItem
{
public:
    explicit StereotypesItem(QGraphicsItem *parent = nullptr);
    ~StereotypesItem() override;

    void setStereotypes(const QList<QString> &stereotypes);

    static QString format(const QList<QString> &stereotypes);
};

} // namespace qmt
