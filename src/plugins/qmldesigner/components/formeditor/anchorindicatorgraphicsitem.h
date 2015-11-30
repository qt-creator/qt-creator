/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef QMLDESIGNER_ANCHORINDICATORGRAPHICSITEM_H
#define QMLDESIGNER_ANCHORINDICATORGRAPHICSITEM_H

#include <QGraphicsObject>

#include <qmlanchors.h>

namespace QmlDesigner {

class AnchorIndicatorGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit AnchorIndicatorGraphicsItem(QGraphicsItem *parent = 0);


    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QRectF boundingRect() const;

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
    AnchorLineType m_sourceAnchorLineType;
    AnchorLineType m_targetAnchorLineType;
    QRectF m_boundingRect;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_ANCHORINDICATORGRAPHICSITEM_H
