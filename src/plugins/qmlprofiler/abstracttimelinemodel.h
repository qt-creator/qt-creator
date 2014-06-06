/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ABSTRACTTIMELINEMODEL_H
#define ABSTRACTTIMELINEMODEL_H


#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include <QObject>
#include <QVariant>
#include <QColor>

namespace QmlProfiler {

class QMLPROFILER_EXPORT AbstractTimelineModel : public QObject
{
    Q_OBJECT

public:
    class AbstractTimelineModelPrivate;
    ~AbstractTimelineModel();

    // Trivial methods implemented by the abstract model itself
    void setModelManager(QmlProfilerModelManager *modelManager);
    QString name() const;
    bool isEmpty() const;

    // Methods are directly passed on to the private model and relying on its virtual methods.
    Q_INVOKABLE qint64 lastTimeMark() const;
    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;
    Q_INVOKABLE int getState() const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;
    int count() const;

    // Methods that have to be implemented by child models
    virtual bool expanded() const = 0;
    virtual void setExpanded(bool expanded) = 0;
    virtual int rowCount() const = 0;
    virtual const QString title() const = 0;
    Q_INVOKABLE virtual int getEventId(int index) const = 0;
    Q_INVOKABLE virtual QColor getColor(int index) const = 0;
    virtual const QVariantList getLabels() const = 0;
    Q_INVOKABLE virtual const QVariantList getEventDetails(int index) const = 0;
    virtual int getEventType(int index) const = 0;
    virtual int getEventRow(int index) const = 0;
    virtual void loadData() = 0;
    virtual void clear() = 0;

    // Methods which can optionally be implemented by child models.
    // returned map should contain "file", "line", "column" properties, or be empty
    Q_INVOKABLE virtual const QVariantMap getEventLocation(int index) const;
    Q_INVOKABLE virtual int getEventIdForHash(const QString &eventHash) const;
    Q_INVOKABLE virtual int getEventIdForLocation(const QString &filename, int line, int column) const;
    Q_INVOKABLE virtual int getBindingLoopDest(int index) const;
    Q_INVOKABLE virtual float getHeight(int index) const;

signals:
    void dataAvailable();
    void stateChanged();
    void emptyChanged();
    void expandedChanged();

protected:
    enum BoxColorProperties {
        EventHueMultiplier = 25,
        FractionHueMultiplier = 96,
        FractionHueMininimum = 10,
        Saturation = 150,
        Lightness = 166
    };

    QColor getEventColor(int index) const
    {
        return getColorByHue(getEventId(index) * EventHueMultiplier);
    }

    QColor getFractionColor(double fraction) const
    {
        return getColorByHue(fraction * FractionHueMultiplier + FractionHueMininimum);
    }

    QColor getColorByHue(int hue) const
    {
        return QColor::fromHsl(hue % 360, Saturation, Lightness);
    }

    explicit AbstractTimelineModel(AbstractTimelineModelPrivate *dd, const QString &name,
                                   QObject *parent = 0);
    AbstractTimelineModelPrivate *d_ptr;

protected slots:
    void dataChanged();

private:
    Q_DECLARE_PRIVATE(AbstractTimelineModel)
};

}

#endif // ABSTRACTTIMELINEMODEL_H
