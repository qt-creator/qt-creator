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
