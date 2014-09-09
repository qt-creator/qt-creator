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
#include "sortedtimelinemodel.h"
#include <QVariant>
#include <QColor>

namespace QmlProfiler {

class QMLPROFILER_EXPORT AbstractTimelineModel : public SortedTimelineModel
{
    Q_OBJECT

public:
    class AbstractTimelineModelPrivate;
    ~AbstractTimelineModel();

    // Trivial methods implemented by the abstract model itself
    void setModelManager(QmlProfilerModelManager *modelManager);
    bool isEmpty() const;

    // Methods are directly passed on to the private model and relying on its virtual methods.
    int rowHeight(int rowNumber) const;
    int rowOffset(int rowNumber) const;
    void setRowHeight(int rowNumber, int height);
    int height() const;

    qint64 traceStartTime() const;
    qint64 traceEndTime() const;
    qint64 traceDuration() const;
    bool accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const;
    bool expanded() const;
    void setExpanded(bool expanded);
    QString displayName() const;

    // Methods that have to be implemented by child models
    virtual int rowCount() const = 0;
    virtual int eventId(int index) const = 0;
    virtual QColor color(int index) const = 0;
    virtual QVariantList labels() const = 0;
    virtual QVariantMap details(int index) const = 0;
    virtual int row(int index) const = 0;
    virtual void loadData() = 0;
    virtual quint64 features() const = 0;

    // Methods which can optionally be implemented by child models.
    // returned map should contain "file", "line", "column" properties, or be empty
    virtual QVariantMap location(int index) const;
    virtual int eventIdForTypeIndex(int typeIndex) const;
    virtual int eventIdForLocation(const QString &filename, int line, int column) const;
    virtual int bindingLoopDest(int index) const;
    virtual float relativeHeight(int index) const;
    virtual int rowMinValue(int rowNumber) const;
    virtual int rowMaxValue(int rowNumber) const;
    virtual void clear();

signals:
    void expandedChanged();
    void rowHeightChanged();

protected:
    static const int DefaultRowHeight = 30;

    enum BoxColorProperties {
        EventHueMultiplier = 25,
        FractionHueMultiplier = 96,
        FractionHueMininimum = 10,
        Saturation = 150,
        Lightness = 166
    };

    QColor colorByEventId(int index) const
    {
        return colorByHue(eventId(index) * EventHueMultiplier);
    }

    QColor colorByFraction(double fraction) const
    {
        return colorByHue(fraction * FractionHueMultiplier + FractionHueMininimum);
    }

    QColor colorByHue(int hue) const
    {
        return QColor::fromHsl(hue % 360, Saturation, Lightness);
    }

    explicit AbstractTimelineModel(AbstractTimelineModelPrivate *dd, const QString &displayName,
                                   QmlDebug::Message message, QmlDebug::RangeType rangeType,
                                   QObject *parent);
    AbstractTimelineModelPrivate *d_ptr;

protected slots:
    void dataChanged();

private:
    Q_DECLARE_PRIVATE(AbstractTimelineModel)
};

}

#endif // ABSTRACTTIMELINEMODEL_H
