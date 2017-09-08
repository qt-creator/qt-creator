/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofileranimationsmodel.h"
#include "qmlprofilermodelmanager.h"

#include <utils/qtcassert.h>
#include <timeline/timelineformattime.h>

#include <QCoreApplication>
#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>


namespace QmlProfiler {
namespace Internal {

QmlProfilerAnimationsModel::QmlProfilerAnimationsModel(QmlProfilerModelManager *manager,
                                                       QObject *parent) :
    QmlProfilerTimelineModel(manager, Event, MaximumRangeType, ProfileAnimations, parent)
{
    m_minNextStartTimes[0] = m_minNextStartTimes[1] = 0;
}

void QmlProfilerAnimationsModel::clear()
{
    m_minNextStartTimes[0] = m_minNextStartTimes[1] = 0;
    m_maxGuiThreadAnimations = m_maxRenderThreadAnimations = 0;
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

bool QmlProfilerAnimationsModel::accepted(const QmlEventType &type) const
{
    return QmlProfilerTimelineModel::accepted(type) && type.detailType() == AnimationFrame;
}

void QmlProfilerAnimationsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_UNUSED(type);
    AnimationThread lastThread = (AnimationThread)event.number<qint32>(2);

    // initial estimation of the event duration: 1/framerate
    qint64 estimatedDuration = event.number<qint32>(0) > 0 ? 1e9 / event.number<qint32>(0) : 1;

    // the profiler registers the animation events at the end of them
    qint64 realEndTime = event.timestamp();

    // ranges should not overlap. If they do, our estimate wasn't accurate enough
    qint64 realStartTime = qMax(event.timestamp() - estimatedDuration,
                                m_minNextStartTimes[lastThread]);

    // Sometimes our estimate is far off or the server has miscalculated the frame rate
    if (realStartTime >= realEndTime)
        realEndTime = realStartTime + 1;

    // Don't "fix" the framerate even if we've fixed the duration.
    // The server should know better after all and if it doesn't we want to see that.
    QmlPaintEventData lastEvent;
    lastEvent.typeId = event.typeIndex();
    lastEvent.framerate = event.number<qint32>(0);
    lastEvent.animationcount = event.number<qint32>(1);

    m_data.insert(insert(realStartTime, realEndTime - realStartTime, lastThread), lastEvent);

    if (lastThread == GuiThread)
        m_maxGuiThreadAnimations = qMax(lastEvent.animationcount, m_maxGuiThreadAnimations);
    else
        m_maxRenderThreadAnimations = qMax(lastEvent.animationcount,
                                           m_maxRenderThreadAnimations);

    m_minNextStartTimes[lastThread] = event.timestamp() + 1;
}

void QmlProfilerAnimationsModel::finalize()
{
    computeNesting();
    setExpandedRowCount((m_maxGuiThreadAnimations == 0 || m_maxRenderThreadAnimations == 0) ? 2 : 3);
    setCollapsedRowCount(expandedRowCount());
}

int QmlProfilerAnimationsModel::rowFromThreadId(int threadId) const
{
    return (threadId == GuiThread || m_maxGuiThreadAnimations == 0) ? 1 : 2;
}

int QmlProfilerAnimationsModel::rowMaxValue(int rowNumber) const
{
    switch (rowNumber) {
    case 1:
        return m_maxGuiThreadAnimations > 0 ? m_maxGuiThreadAnimations : m_maxRenderThreadAnimations;
    case 2:
        return m_maxRenderThreadAnimations;
    default:
        return QmlProfilerTimelineModel::rowMaxValue(rowNumber);
    }
}

int QmlProfilerAnimationsModel::typeId(int index) const
{
    return m_data[index].typeId;
}

int QmlProfilerAnimationsModel::expandedRow(int index) const
{
    return rowFromThreadId(selectionId(index));
}

int QmlProfilerAnimationsModel::collapsedRow(int index) const
{
    return rowFromThreadId(selectionId(index));
}

QRgb QmlProfilerAnimationsModel::color(int index) const
{
    double fpsFraction = m_data[index].framerate / 60.0;
    if (fpsFraction > 1.0)
        fpsFraction = 1.0;
    if (fpsFraction < 0.0)
        fpsFraction = 0.0;
    return colorByFraction(fpsFraction);
}

float QmlProfilerAnimationsModel::relativeHeight(int index) const
{
    return (float)m_data[index].animationcount / (float)(selectionId(index) == GuiThread ?
                                                             m_maxGuiThreadAnimations :
                                                             m_maxRenderThreadAnimations);
}

QVariantList QmlProfilerAnimationsModel::labels() const
{
    QVariantList result;

    if (m_maxGuiThreadAnimations > 0) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), tr("Animations"));
        element.insert(QLatin1String("description"), tr("GUI Thread"));
        element.insert(QLatin1String("id"), GuiThread);
        result << element;
    }

    if (m_maxRenderThreadAnimations > 0) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), tr("Animations"));
        element.insert(QLatin1String("description"), tr("Render Thread"));
        element.insert(QLatin1String("id"), RenderThread);
        result << element;
    }

    return result;
}

QVariantMap QmlProfilerAnimationsModel::details(int index) const
{
    QVariantMap result;

    result.insert(QStringLiteral("displayName"), displayName());
    result.insert(tr("Duration"), Timeline::formatTime(duration(index)));
    result.insert(tr("Framerate"), QString::fromLatin1("%1 FPS").arg(m_data[index].framerate));
    result.insert(tr("Animations"), QString::number(m_data[index].animationcount));
    result.insert(tr("Context"), selectionId(index) == GuiThread ? tr("GUI Thread") :
                                                                   tr("Render Thread"));
    return result;
}

} // namespace Internal
} // namespace QmlProfiler
