/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
    explicit CustomIconItem(DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = 0);
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

private:
    DiagramSceneModel *m_diagramSceneModel = 0;
    QString m_stereotypeIconId;
    StereotypeIcon m_stereotypeIcon;
    QSizeF m_baseSize;
    QSizeF m_actualSize;
    QBrush m_brush;
    QPen m_pen;
};

} // namespace qmt
