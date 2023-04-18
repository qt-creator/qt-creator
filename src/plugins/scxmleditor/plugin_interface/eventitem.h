// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"
#include "textitem.h"

namespace ScxmlEditor::PluginInterface {

class EventItem : public BaseItem
{
public:
    explicit EventItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    void updateAttributes() override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}

private:
    TextItem *m_eventNameItem;
};

class OnEntryExitItem : public BaseItem
{
public:
    explicit OnEntryExitItem(BaseItem *parent = nullptr);

    int type() const override { return StateWarningType; }

    void updateAttributes() override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}
    void finalizeCreation() override;
    void addChild(ScxmlTag *tag) override;

    QRectF childBoundingRect() const;

private:
    TextItem *m_eventNameItem;
};

} // namespace ScxmlEditor::PluginInterface
