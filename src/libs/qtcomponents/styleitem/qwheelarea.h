/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QWHEELAREA_H
#define QWHEELAREA_H


#include <qobject.h>
#include <qdeclarative.h>
#include <qdeclarativeitem.h>
#include <qcoreevent.h>
#include <qevent.h>
#include <qgraphicssceneevent.h>

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
