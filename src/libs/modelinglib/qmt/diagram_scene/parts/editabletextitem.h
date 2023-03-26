// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsTextItem>

namespace qmt {

class EditableTextItem : public QGraphicsTextItem
{
    Q_OBJECT

public:
    explicit EditableTextItem(QGraphicsItem *parent);
    ~EditableTextItem() override;

signals:
    void returnKeyPressed();

public:
    void setShowFocus(bool showFocus);
    void setFilterReturnKey(bool filterReturnKey);
    void setFilterTabKey(bool filterTabKey);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void selectAll();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    bool isReturnKey(QKeyEvent *event) const;

private:
    bool m_showFocus = false;
    bool m_filterReturnKey = false;
    bool m_filterTabKey = false;
};

} // namespace qmt
