/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "debugmessagesmodel.h"

using namespace QmlProfiler;

namespace QmlProfilerExtension {
namespace Internal {

bool DebugMessagesModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    return event.message == QmlDebug::DebugMessage;
}

DebugMessagesModel::DebugMessagesModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, QmlDebug::DebugMessage, QmlDebug::MaximumRangeType,
                             QmlDebug::ProfileDebugMessages, parent), m_maximumMsgType(-1)
{
}

int DebugMessagesModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QColor DebugMessagesModel::color(int index) const
{
    return colorBySelectionId(index);
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Debug Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Warning Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Critical Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Fatal Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Info Message"),
};

QString DebugMessagesModel::messageType(uint i)
{
    return i < sizeof(messageTypes) / sizeof(char *) ? tr(messageTypes[i]) :
                                                       tr("Unknown Message %1").arg(i);
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
    const QmlProfilerDataModel::QmlEventTypeData &type =
            modelManager()->qmlModel()->getEventTypes()[m_data[index].typeId];

    QVariantMap result;
    result.insert(QLatin1String("displayName"), messageType(type.detailType));
    result.insert(tr("Timestamp"), QmlProfilerDataModel::formatTime(startTime(index)));
    result.insert(tr("Message"), m_data[index].text);
    result.insert(tr("Location"), type.displayName);
    return result;
}

int DebugMessagesModel::expandedRow(int index) const
{
    return selectionId(index) + 1;
}

int DebugMessagesModel::collapsedRow(int index) const
{
    Q_UNUSED(index);
    return 1;
}

void DebugMessagesModel::loadData()
{
    QmlProfilerDataModel *simpleModel = modelManager()->qmlModel();
    if (simpleModel->isEmpty())
        return;

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();

    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex()];
        if (!accepted(type) || event.startTime() < 0)
            continue;

        m_data.insert(insert(event.startTime(), 0, type.detailType),
                      MessageData(event.stringData(), event.typeIndex()));
        if (type.detailType > m_maximumMsgType)
            m_maximumMsgType = event.typeIndex();
        updateProgress(count(), simpleModel->getEvents().count());
    }
    setCollapsedRowCount(2);
    setExpandedRowCount(m_maximumMsgType + 2);
    updateProgress(1, 1);
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

}
}
