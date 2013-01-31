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

#ifndef QRANGEMODEL_H
#define QRANGEMODEL_H

#include <qobject.h>
#include <qgraphicsitem.h>
#include <qabstractslider.h>
#include <qdeclarative.h>

class QRangeModelPrivate;

class QRangeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(qreal minimumValue READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(qreal maximumValue READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qreal positionAtMinimum READ positionAtMinimum WRITE setPositionAtMinimum NOTIFY positionAtMinimumChanged)
    Q_PROPERTY(qreal positionAtMaximum READ positionAtMaximum WRITE setPositionAtMaximum NOTIFY positionAtMaximumChanged)
    Q_PROPERTY(bool inverted READ inverted WRITE setInverted NOTIFY invertedChanged)

public:
    QRangeModel(QObject *parent = 0);
    virtual ~QRangeModel();

    void setRange(qreal min, qreal max);
    void setPositionRange(qreal min, qreal max);

    void setStepSize(qreal stepSize);
    qreal stepSize() const;

    void setMinimum(qreal min);
    qreal minimum() const;

    void setMaximum(qreal max);
    qreal maximum() const;

    void setPositionAtMinimum(qreal posAtMin);
    qreal positionAtMinimum() const;

    void setPositionAtMaximum(qreal posAtMax);
    qreal positionAtMaximum() const;

    void setInverted(bool inverted);
    bool inverted() const;

    qreal value() const;
    qreal position() const;

    Q_INVOKABLE qreal valueForPosition(qreal position) const;
    Q_INVOKABLE qreal positionForValue(qreal value) const;

public Q_SLOTS:
    void toMinimum();
    void toMaximum();
    void setValue(qreal value);
    void setPosition(qreal position);

Q_SIGNALS:
    void valueChanged(qreal value);
    void positionChanged(qreal position);

    void stepSizeChanged(qreal stepSize);

    void invertedChanged(bool inverted);

    void minimumChanged(qreal min);
    void maximumChanged(qreal max);
    void positionAtMinimumChanged(qreal min);
    void positionAtMaximumChanged(qreal max);

protected:
    QRangeModel(QRangeModelPrivate &dd, QObject *parent);
    QRangeModelPrivate* d_ptr;

private:
    Q_DISABLE_COPY(QRangeModel)
    Q_DECLARE_PRIVATE(QRangeModel)

};

QML_DECLARE_TYPE(QRangeModel)

#endif // QRANGEMODEL_H
