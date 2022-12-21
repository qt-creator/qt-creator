// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsObject>

#include <qmlanchors.h>

namespace QmlDesigner {

class AnchorIndicatorGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit AnchorIndicatorGraphicsItem(QGraphicsItem *parent = nullptr);


    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;

    void updateAnchorIndicator(const AnchorLine &sourceAnchorLine, const AnchorLine &targetAnchorLine);

    AnchorLineType sourceAnchorLineType() const;
    void setSourceAnchorLineType(const AnchorLineType &sourceAnchorLineType);

protected:
    void updateBoundingRect();

private:
    QPointF m_startPoint;
    QPointF m_firstControlPoint;
    QPointF m_secondControlPoint;
    QPointF m_endPoint;
    QPointF m_sourceAnchorLineFirstPoint;
    QPointF m_sourceAnchorLineSecondPoint;
    QPointF m_targetAnchorLineFirstPoint;
    QPointF m_targetAnchorLineSecondPoint;
    AnchorLineType m_sourceAnchorLineType = AnchorLineInvalid;
    AnchorLineType m_targetAnchorLineType = AnchorLineInvalid;
    QRectF m_boundingRect;
};

} // namespace QmlDesigner
