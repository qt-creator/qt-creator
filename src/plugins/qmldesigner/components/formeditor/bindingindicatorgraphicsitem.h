// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsObject>

namespace QmlDesigner {

class BindingIndicatorGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit BindingIndicatorGraphicsItem(QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;

    void updateBindingIndicator(const QLineF &bindingLine);

private:
    QLineF m_bindingLine;

};

} // namespace QmlDesigner
