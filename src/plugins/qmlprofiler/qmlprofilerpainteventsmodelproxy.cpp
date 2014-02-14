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

#include "qmlprofilerpainteventsmodelproxy.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersimplemodel.h"
#include "sortedtimelinemodel.h"
#include "singlecategorytimelinemodel_p.h"
#include <QCoreApplication>

#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

struct CategorySpan {
    bool expanded;
    int expandedRows;
    int contractedRows;
};

class PaintEventsModelProxy::PaintEventsModelProxyPrivate :
        public SortedTimelineModel<QmlPaintEventData,
                                   SingleCategoryTimelineModel::SingleCategoryTimelineModelPrivate>
{
public:
    void computeAnimationCountLimit();

    int minAnimationCount;
    int maxAnimationCount;
    bool seenForeignPaintEvent;

private:
    Q_DECLARE_PUBLIC(PaintEventsModelProxy)
};

PaintEventsModelProxy::PaintEventsModelProxy(QObject *parent)
    : SingleCategoryTimelineModel(new PaintEventsModelProxyPrivate,
                                  QLatin1String("PaintEventsModelProxy"), tr("Painting"),
                                  QmlDebug::Painting, parent)
{
}


void PaintEventsModelProxy::clear()
{
    Q_D(PaintEventsModelProxy);
    d->SortedTimelineModel::clear();
    d->minAnimationCount = 1;
    d->maxAnimationCount = 1;
    d->expanded = false;
    d->seenForeignPaintEvent = false;
    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

bool PaintEventsModelProxy::eventAccepted(const QmlProfilerSimpleModel::QmlEventData &event) const
{
    return SingleCategoryTimelineModel::eventAccepted(event) &&
            event.bindingType == QmlDebug::AnimationFrame;
}

void PaintEventsModelProxy::loadData()
{
    Q_D(PaintEventsModelProxy);
    clear();
    QmlProfilerSimpleModel *simpleModel = d->modelManager->simpleModel();
    if (simpleModel->isEmpty())
        return;

    // collect events
    const QVector<QmlProfilerSimpleModel::QmlEventData> referenceList = simpleModel->getEvents();

    QmlPaintEventData lastEvent;
    qint64 minNextStartTime = 0;

    foreach (const QmlProfilerSimpleModel::QmlEventData &event, referenceList) {
        if (!eventAccepted(event)) {
            if (event.eventType == QmlDebug::Painting)
                d->seenForeignPaintEvent = true;
            continue;
        }

        // initial estimation of the event duration: 1/framerate
        qint64 estimatedDuration = event.numericData1 > 0 ? 1e9/event.numericData1 : 1;

        // the profiler registers the animation events at the end of them
        qint64 realEndTime = event.startTime;

        // ranges should not overlap. If they do, our estimate wasn't accurate enough
        qint64 realStartTime = qMax(event.startTime - estimatedDuration, minNextStartTime);

        // Sometimes our estimate is far off or the server has miscalculated the frame rate
        if (realStartTime >= realEndTime)
            realEndTime = realStartTime + 1;

        // Don't "fix" the framerate even if we've fixed the duration.
        // The server should know better after all and if it doesn't we want to see that.
        lastEvent.framerate = (int)event.numericData1;
        lastEvent.animationcount = (int)event.numericData2;
        d->insert(realStartTime, realEndTime - realStartTime, lastEvent);

        minNextStartTime = event.startTime + 1;

        d->modelManager->modelProxyCountUpdated(d->modelId, d->count(), referenceList.count());
    }

    d->computeAnimationCountLimit();
    d->computeNesting();

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

/////////////////// QML interface

int PaintEventsModelProxy::categoryDepth(int categoryIndex) const
{
    Q_D(const PaintEventsModelProxy);
    Q_UNUSED(categoryIndex);
    if (isEmpty())
        return d->seenForeignPaintEvent ? 0 : 1;
    else
        return 2;
}

int PaintEventsModelProxy::getEventRow(int index) const
{
    Q_UNUSED(index);
    return 1;
}

int PaintEventsModelProxy::getEventId(int index) const
{
    // there is only one event Id for all painting events
    Q_UNUSED(index);
    return 0;
}

QColor PaintEventsModelProxy::getColor(int index) const
{
    Q_D(const PaintEventsModelProxy);
    double fpsFraction = d->range(index).framerate / 60.0;
    if (fpsFraction > 1.0)
        fpsFraction = 1.0;
    if (fpsFraction < 0.0)
        fpsFraction = 0.0;
    return QColor::fromHsl((fpsFraction*96)+10, 76, 166);
}

float PaintEventsModelProxy::getHeight(int index) const
{
    Q_D(const PaintEventsModelProxy);
    float scale = d->maxAnimationCount - d->minAnimationCount;
    float fraction = 1.0f;
    if (scale > 1)
        fraction = (float)(d->range(index).animationcount -
                            d->minAnimationCount) / scale;

    return fraction * 0.85f + 0.15f;
}

const QVariantList PaintEventsModelProxy::getLabelsForCategory(int category) const
{
    Q_UNUSED(category);
    QVariantList result;

    if (!isEmpty()) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), QVariant(QLatin1String("Animations")));
        element.insert(QLatin1String("description"), QVariant(QLatin1String("Animations")));
        element.insert(QLatin1String("id"), QVariant(0));
        result << element;
    }

    return result;
}

void PaintEventsModelProxy::PaintEventsModelProxyPrivate::computeAnimationCountLimit()
{
    minAnimationCount = 1;
    maxAnimationCount = 1;
    if (count() == 0)
        return;

    for (int i=0; i < count(); i++) {
        if (range(i).animationcount < minAnimationCount)
            minAnimationCount = range(i).animationcount;
        else if (range(i).animationcount > maxAnimationCount)
            maxAnimationCount = range(i).animationcount;
    }
}

const QVariantList PaintEventsModelProxy::getEventDetails(int index) const
{
    Q_D(const PaintEventsModelProxy);
    QVariantList result;
//    int eventId = getEventId(index);

    static const char trContext[] = "RangeDetails";
    {
        QVariantMap valuePair;
        valuePair.insert(QLatin1String("title"), QVariant(categoryLabel(0)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Duration:"),
                         QVariant(QmlProfilerSimpleModel::formatTime(d->range(index).duration)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Framerate:"), QVariant(QString::fromLatin1("%1 FPS").arg(d->range(index).framerate)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Animations:"), QVariant(QString::fromLatin1("%1").arg(d->range(index).animationcount)));
        result << valuePair;
    }

    return result;
}

}
}
