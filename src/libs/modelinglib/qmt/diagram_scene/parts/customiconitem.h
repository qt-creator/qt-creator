// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

#include "qmt/stereotype/iconshape.h"
#include "qmt/stereotype/stereotypeicon.h"

#include <QBrush>
#include <QPen>

namespace qmt {

class DiagramSceneModel;

class CustomIconItem : public QGraphicsItem
{
public:
    explicit CustomIconItem(DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = nullptr);
    ~CustomIconItem() override;

    void setStereotypeIconId(const QString &stereotypeIconId);
    void setBaseSize(const QSizeF &baseSize);
    void setActualSize(const QSizeF &actualSize);
    void setBrush(const QBrush &brush);
    void setPen(const QPen &pen);
    StereotypeIcon stereotypeIcon() const { return m_stereotypeIcon; }
    double shapeWidth() const;
    double shapeHeight() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QList<QPolygonF> outline() const;

private:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    QString m_stereotypeIconId;
    StereotypeIcon m_stereotypeIcon;
    QSizeF m_baseSize;
    QSizeF m_actualSize;
    QBrush m_brush;
    QPen m_pen;
};

} // namespace qmt
