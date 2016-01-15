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

#include "qmlprofilerrangemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "qmlprofilerbindingloopsrenderpass.h"
#include "timeline/timelinenotesrenderpass.h"
#include "timeline/timelineitemsrenderpass.h"
#include "timeline/timelineselectionrenderpass.h"

#include <QCoreApplication>
#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {


QmlProfilerRangeModel::QmlProfilerRangeModel(QmlProfilerModelManager *manager,
                                             QmlDebug::RangeType range, QObject *parent) :
    QmlProfilerTimelineModel(manager, QmlDebug::MaximumMessage, range,
                             QmlDebug::featureFromRangeType(range), parent)
{
    m_expandedRowTypes << -1;
}

void QmlProfilerRangeModel::clear()
{
    m_expandedRowTypes.clear();
    m_expandedRowTypes << -1;
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

bool QmlProfilerRangeModel::supportsBindingLoops() const
{
    return rangeType() == QmlDebug::Binding || rangeType() == QmlDebug::HandlingSignal;
}

void QmlProfilerRangeModel::loadData()
{
    QmlProfilerDataModel *simpleModel = modelManager()->qmlModel();
    if (simpleModel->isEmpty())
        return;

    // collect events
    const QVector<QmlProfilerDataModel::QmlEventData> &eventList = simpleModel->getEvents();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typesList = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        const QmlProfilerDataModel::QmlEventTypeData &type = typesList[event.typeIndex()];
        if (!accepted(type))
            continue;

        // store starttime-based instance
        m_data.insert(insert(event.startTime(), event.duration(), event.typeIndex()),
                      QmlRangeEventStartInstance());
        updateProgress(count(), eventList.count() * 5);
    }

    updateProgress(1, 5);

    // compute range nesting
    computeNesting();

    updateProgress(2, 5);

    // compute nestingLevel - nonexpanded
    computeNestingContracted();

    updateProgress(3, 5);

    // compute nestingLevel - expanded
    computeExpandedLevels();

    updateProgress(4, 5);

    if (supportsBindingLoops())
        findBindingLoops();

    updateProgress(1, 1);
}

void QmlProfilerRangeModel::computeNestingContracted()
{
    int i;
    int eventCount = count();

    int nestingLevels = QmlDebug::Constants::QML_MIN_LEVEL;
    int collapsedRowCount = nestingLevels + 1;
    QVector<qint64> nestingEndTimes;
    nestingEndTimes.fill(0, nestingLevels + 1);

    for (i = 0; i < eventCount; i++) {
        qint64 st = startTime(i);

        // per type
        if (nestingEndTimes[nestingLevels] > st) {
            if (++nestingLevels == nestingEndTimes.size())
                nestingEndTimes << 0;
            if (nestingLevels == collapsedRowCount)
                ++collapsedRowCount;
        } else {
            while (nestingLevels > QmlDebug::Constants::QML_MIN_LEVEL &&
                   nestingEndTimes[nestingLevels-1] <= st)
                nestingLevels--;
        }
        nestingEndTimes[nestingLevels] = st + duration(i);

        m_data[i].displayRowCollapsed = nestingLevels;
    }
    setCollapsedRowCount(collapsedRowCount);
}

void QmlProfilerRangeModel::computeExpandedLevels()
{
    QHash<int, int> eventRow;
    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        int eventTypeId = typeId(i);
        if (!eventRow.contains(eventTypeId)) {
            eventRow[eventTypeId] = m_expandedRowTypes.size();
            m_expandedRowTypes << eventTypeId;
        }
        m_data[i].displayRowExpanded = eventRow[eventTypeId];
    }
    setExpandedRowCount(m_expandedRowTypes.size());
}

void QmlProfilerRangeModel::findBindingLoops()
{
    typedef QPair<int, int> CallStackEntry;
    QStack<CallStackEntry> callStack;

    for (int i = 0; i < count(); ++i) {
        int potentialParent = callStack.isEmpty() ? -1 : callStack.top().second;

        while (potentialParent != -1 && !(endTime(potentialParent) > startTime(i))) {
            callStack.pop();
            potentialParent = callStack.isEmpty() ? -1 : callStack.top().second;
        }

        // check whether event is already in stack
        for (int ii = 0; ii < callStack.size(); ++ii) {
            if (callStack.at(ii).first == typeId(i)) {
                m_data[i].bindingLoopHead = callStack.at(ii).second;
                break;
            }
        }

        CallStackEntry newEntry(typeId(i), i);
        callStack.push(newEntry);
    }

}

int QmlProfilerRangeModel::expandedRow(int index) const
{
    return m_data[index].displayRowExpanded;
}

int QmlProfilerRangeModel::collapsedRow(int index) const
{
    return m_data[index].displayRowCollapsed;
}

int QmlProfilerRangeModel::bindingLoopDest(int index) const
{
    return m_data[index].bindingLoopHead;
}

QColor QmlProfilerRangeModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList QmlProfilerRangeModel::labels() const
{
    QVariantList result;

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            modelManager()->qmlModel()->getEventTypes();
    for (int i = 1; i < expandedRowCount(); i++) { // Ignore the -1 for the first row
        QVariantMap element;
        int typeId = m_expandedRowTypes[i];
        element.insert(QLatin1String("displayName"), QVariant(types[typeId].displayName));
        element.insert(QLatin1String("description"), QVariant(types[typeId].data));
        element.insert(QLatin1String("id"), QVariant(typeId));
        result << element;
    }

    return result;
}

QVariantMap QmlProfilerRangeModel::details(int index) const
{
    QVariantMap result;
    int id = selectionId(index);
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            modelManager()->qmlModel()->getEventTypes();

    result.insert(QStringLiteral("displayName"),
                  tr(QmlProfilerModelManager::featureName(mainFeature())));
    result.insert(tr("Duration"), QmlProfilerDataModel::formatTime(duration(index)));

    result.insert(tr("Details"), types[id].data);
    result.insert(tr("Location"), types[id].displayName);
    return result;
}

QVariantMap QmlProfilerRangeModel::location(int index) const
{
    return locationFromTypeId(index);
}

int QmlProfilerRangeModel::typeId(int index) const
{
    return selectionId(index);
}

int QmlProfilerRangeModel::selectionIdForLocation(const QString &filename, int line, int column) const
{
    // if this is called from v8 view, we don't have the column number, it will be -1
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            modelManager()->qmlModel()->getEventTypes();
    for (int i = 1; i < expandedRowCount(); ++i) {
        int typeId = m_expandedRowTypes[i];
        const QmlProfilerDataModel::QmlEventTypeData &eventData = types[typeId];
        if (eventData.location.filename == filename &&
                eventData.location.line == line &&
                (column == -1 || eventData.location.column == column))
            return typeId;
    }
    return -1;
}

QList<const Timeline::TimelineRenderPass *> QmlProfilerRangeModel::supportedRenderPasses() const
{
    if (supportsBindingLoops()) {
        QList<const Timeline::TimelineRenderPass *> passes;
        passes << Timeline::TimelineItemsRenderPass::instance()
               << QmlProfilerBindingLoopsRenderPass::instance()
               << Timeline::TimelineSelectionRenderPass::instance()
               << Timeline::TimelineNotesRenderPass::instance();
        return passes;
    } else {
        return QmlProfilerTimelineModel::supportedRenderPasses();
    }

}

}
}
