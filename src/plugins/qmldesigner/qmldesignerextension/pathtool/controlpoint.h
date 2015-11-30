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
#ifndef QMLDESIGNER_CONTROLPOINT_H
#define QMLDESIGNER_CONTROLPOINT_H

#include <modelnode.h>

#include <QPointF>
#include <QExplicitlySharedDataPointer>

namespace QmlDesigner {

enum PointType {
    StartPoint,
    FirstControlPoint,
    SecondControlPoint,
    EndPoint,
    StartAndEndPoint
};

class ControlPointData : public QSharedData
{
public:
    ModelNode pathElementModelNode;
    ModelNode pathModelNode;
    QPointF coordinate;
    PointType pointType;
};

class ControlPoint
{
    friend bool operator ==(const ControlPoint& firstControlPoint, const ControlPoint& secondControlPoint);

public:
    ControlPoint();
    ControlPoint(const ControlPoint &other);
    ControlPoint(const QPointF &coordinate);
    ControlPoint(double x, double y);

    ~ControlPoint();

    ControlPoint &operator =(const ControlPoint &other);

    void setX(double x);
    void setY(double y);
    void setCoordinate(const QPointF &coordinate);
    QPointF coordinate() const;

    void setPathElementModelNode(const ModelNode &pathElementModelNode);
    ModelNode pathElementModelNode() const;

    void setPathModelNode(const ModelNode &pathModelNode);
    ModelNode pathModelNode() const;

    void setPointType(PointType pointType);
    PointType pointType() const;

    bool isValid() const;
    bool isEditPoint() const;
    bool isControlVertex() const;

    void updateModelNode();

private:
    QExplicitlySharedDataPointer<ControlPointData> d;
};

bool operator ==(const ControlPoint& firstControlPoint, const ControlPoint& secondControlPoint);
QDebug operator<<(QDebug debug, const ControlPoint &controlPoint);

} // namespace QmlDesigner

#endif // QMLDESIGNER_CONTROLPOINT_H
