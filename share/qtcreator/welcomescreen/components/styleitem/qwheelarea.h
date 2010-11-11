/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project on Qt Labs.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions contained
** in the Technology Preview License Agreement accompanying this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
****************************************************************************/

#ifndef QWHEELAREA_H
#define QWHEELAREA_H


#include <QtCore/qobject.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeitem.h>
#include <QtCore/qcoreevent.h>
#include <QtGui/qevent.h>
#include <QtGui/qgraphicssceneevent.h>

class QWheelArea : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qreal verticalDelta READ verticalDelta WRITE setVerticalDelta NOTIFY verticalWheelMoved)
    Q_PROPERTY(qreal horizontalDelta READ horizontalDelta WRITE setHorizontalDelta NOTIFY horizontalWheelMoved)
    Q_PROPERTY(qreal horizontalMinimumValue READ horizontalMinimumValue WRITE setHorizontalMinimumValue)
    Q_PROPERTY(qreal horizontalMaximumValue READ horizontalMaximumValue WRITE setHorizontalMaximumValue)
    Q_PROPERTY(qreal verticalMinimumValue READ verticalMinimumValue WRITE setVerticalMinimumValue)
    Q_PROPERTY(qreal verticalMaximumValue READ verticalMaximumValue WRITE setVerticalMaximumValue)
    Q_PROPERTY(qreal horizontalValue READ horizontalValue WRITE setHorizontalValue)
    Q_PROPERTY(qreal verticalValue READ verticalValue WRITE setVerticalValue)


public:
    QWheelArea(QDeclarativeItem *parent = 0);

    virtual ~QWheelArea();

    virtual bool event (QEvent * e);

    void setHorizontalMinimumValue(qreal min);
    qreal horizontalMinimumValue() const;

    void setHorizontalMaximumValue(qreal min);
    qreal horizontalMaximumValue() const;

    void setVerticalMinimumValue(qreal min);
    qreal verticalMinimumValue() const;

    void setVerticalMaximumValue(qreal min);
    qreal verticalMaximumValue() const;

    void setHorizontalValue(qreal val);
    qreal horizontalValue() const;

    void setVerticalValue(qreal val);
    qreal verticalValue() const;

    void setVerticalDelta(qreal d);
    qreal verticalDelta() const;

    void setHorizontalDelta(qreal d);
    qreal horizontalDelta() const;

Q_SIGNALS:
    void verticalValueChanged();
    void horizontalValueChanged();
    void verticalWheelMoved();
    void horizontalWheelMoved();

private:
    qreal _horizontalMinimumValue;
    qreal _horizontalMaximumValue;
    qreal _verticalMinimumValue;
    qreal _verticalMaximumValue;
    qreal _horizontalValue;
    qreal _verticalValue;
    qreal _verticalDelta;
    qreal _horizontalDelta;

    Q_DISABLE_COPY(QWheelArea)
};

QML_DECLARE_TYPE(QWheelArea)


#endif // QWHEELAREA_H
