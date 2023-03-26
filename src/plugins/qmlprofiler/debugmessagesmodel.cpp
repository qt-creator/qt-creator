// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugmessagesmodel.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertr.h"
#include <tracing/timelineformattime.h>

namespace QmlProfiler {
namespace Internal {

DebugMessagesModel::DebugMessagesModel(QmlProfilerModelManager *manager,
                                       Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, DebugMessage, UndefinedRangeType, ProfileDebugMessages, parent),
    m_maximumMsgType(-1)
{
}

int DebugMessagesModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QRgb DebugMessagesModel::color(int index) const
{
    return colorBySelectionId(index);
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Debug Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Warning Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Critical Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Fatal Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Info Message"),
};

QString DebugMessagesModel::messageType(uint i)
{
    return i < sizeof(messageTypes) / sizeof(char *) ? Tr::tr(messageTypes[i]) :
                                                       Tr::tr("Unknown Message %1").arg(i);
}

QVariantList DebugMessagesModel::labels() const
{
    QVariantList result;

    for (int i = 0; i <= m_maximumMsgType; ++i) {
        QVariantMap element;
        element.insert(QLatin1String("description"), messageType(i));
        element.insert(QLatin1String("id"), i);
        result << element;
    }
    return result;
}

QVariantMap DebugMessagesModel::details(int index) const
{
    const QmlProfilerModelManager *manager = modelManager();
    const QmlEventType &type = manager->eventType(m_data[index].typeId);

    QVariantMap result;
    result.insert(QLatin1String("displayName"), messageType(type.detailType()));
    result.insert(Tr::tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                        manager->traceDuration()));
    result.insert(Tr::tr("Message"), m_data[index].text);
    result.insert(Tr::tr("Location"), type.displayName());
    return result;
}

int DebugMessagesModel::expandedRow(int index) const
{
    return selectionId(index) + 1;
}

int DebugMessagesModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return Constants::QML_MIN_LEVEL;
}

void DebugMessagesModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    m_data.insert(insert(event.timestamp(), 0, type.detailType()),
                  Item(event.string(), event.typeIndex()));
    if (type.detailType() > m_maximumMsgType)
        m_maximumMsgType = type.detailType();
}

void DebugMessagesModel::finalize()
{
    setCollapsedRowCount(Constants::QML_MIN_LEVEL + 1);
    setExpandedRowCount(m_maximumMsgType + 2);
    QmlProfilerTimelineModel::finalize();
}

void DebugMessagesModel::clear()
{
    m_data.clear();
    m_maximumMsgType = -1;
    QmlProfilerTimelineModel::clear();
}

QVariantMap DebugMessagesModel::location(int index) const
{
    return locationFromTypeId(index);
}

} // namespace Internal
} // namespace QmlProfiler
