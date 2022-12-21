// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"
#include <QFocusEvent>
#include <QGraphicsTextItem>
#include <QKeyEvent>

namespace ScxmlEditor {

namespace PluginInterface {

class TextItem : public QGraphicsTextItem
{
    Q_OBJECT

public:
    explicit TextItem(QGraphicsItem *parent = nullptr);
    explicit TextItem(const QString &id, QGraphicsItem *parent = nullptr);

    int type() const override
    {
        return TextType;
    }

    void setItalic(bool ital);
    void setText(const QString &t);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void textChanged();
    void textReady(const QString &text);
    void selected(bool sel);

private:
    void checkText();
    void init();
    bool needIgnore(const QPointF sPos) const;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
