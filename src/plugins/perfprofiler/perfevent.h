/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include "perfprofiler_global.h"
#include "perfeventtype.h"

#include <tracing/traceevent.h>
#include <utils/qtcassert.h>

#include <QVector>
#include <QDataStream>

namespace PerfProfiler {
namespace Internal {

class PerfEvent : public Timeline::TraceEvent
{
public:
    static const qint32 staticClassId = 0x70657266; // 'perf'

    PerfEvent() : TraceEvent(staticClassId) {}

    enum SpecialTypeId {
        AmbiguousTypeId        =  0,
        InvalidTypeId          = -1,
        ThreadStartTypeId      = -2,
        ThreadEndTypeId        = -3,
        LostTypeId             = -4,
        LastSpecialTypeId      = -5
    };

    const QVector<qint32> &origFrames() const { return m_origFrames; }
    quint8 origNumGuessedFrames() const { return m_origNumGuessedFrames; }

    const QVector<qint32> &frames() const { return m_frames; }
    void setFrames(const QVector<qint32> &frames) { m_frames = frames; }

    const QHash<qint32, QVariant> &traceData() const { return m_traceData; }

    quint32 pid() const { return m_pid; }
    quint32 tid() const { return m_tid; }

    quint8 numGuessedFrames() const { return m_numGuessedFrames; }
    void setNumGuessedFrames(quint8 numGuessedFrames) { m_numGuessedFrames = numGuessedFrames; }

    quint64 period() const { return m_period; }
    quint64 weight() const { return m_weight; }

    quint8 feature() const { return m_feature; }

private:
    friend QDataStream &operator>>(QDataStream &stream, PerfEvent &event);
    friend QDataStream &operator<<(QDataStream &stream, const PerfEvent &event);

    QVector<qint32> m_origFrames;
    QVector<qint32> m_frames;
    QHash<qint32, QVariant> m_traceData;
    quint32 m_pid = 0;
    quint32 m_tid = 0;
    quint64 m_period = 0;
    quint64 m_weight = 0;
    quint8 m_origNumGuessedFrames = 0;
    quint8 m_numGuessedFrames = 0;
    quint8 m_feature = PerfEventType::InvalidFeature;
};

inline QDataStream &operator>>(QDataStream &stream, PerfEvent &event)
{
    stream >> event.m_feature;
    switch (event.m_feature) {
    case PerfEventType::Command:
    case PerfEventType::LocationDefinition:
    case PerfEventType::SymbolDefinition:
    case PerfEventType::AttributesDefinition:
    case PerfEventType::StringDefinition:
    case PerfEventType::FeaturesDefinition:
    case PerfEventType::Error:
    case PerfEventType::Progress:
    case PerfEventType::TracePointFormat:
        return stream; // in fact type data. to be handled elsewhere
    case PerfEventType::ThreadStart:
    case PerfEventType::ThreadEnd:
    case PerfEventType::LostDefinition:
    case PerfEventType::Sample43:
    case PerfEventType::Sample:
    case PerfEventType::TracePointSample:
        break;
    case PerfEventType::InvalidFeature:
        QTC_ASSERT(false, return stream);
    }

    quint64 timestamp;
    stream >> event.m_pid >> event.m_tid >> timestamp;

    static const quint64 qint64Max = static_cast<quint64>(std::numeric_limits<qint64>::max());
    event.setTimestamp(static_cast<qint64>(qMin(timestamp, qint64Max)));

    switch (event.m_feature) {
    case PerfEventType::ThreadStart:
        event.setTypeIndex(PerfEvent::ThreadStartTypeId);
        break;
    case PerfEventType::ThreadEnd:
        event.setTypeIndex(PerfEvent::ThreadEndTypeId);
        break;
    case PerfEventType::LostDefinition:
        event.setTypeIndex(PerfEvent::LostTypeId);
        break;
    default: {
        qint32 typeIndex;
        stream >> event.m_origFrames >> event.m_origNumGuessedFrames >> typeIndex;
        if (event.m_feature != PerfEventType::Sample43) {
            stream >> event.m_period >> event.m_weight;
            if (event.m_feature == PerfEventType::TracePointSample)
                stream >> event.m_traceData;
        }
        event.setTypeIndex(PerfEvent::LastSpecialTypeId - typeIndex);
    }
    }

    return stream;
}

inline QDataStream &operator<<(QDataStream &stream, const PerfEvent &event)
{
    quint8 feature = event.feature();
    stream << feature << event.m_pid << event.m_tid
           << (event.timestamp() < 0ll ? 0ull : static_cast<quint64>(event.timestamp()));
    switch (feature) {
    case PerfEventType::ThreadStart:
    case PerfEventType::ThreadEnd:
    case PerfEventType::LostDefinition:
        break;
    case PerfEventType::Sample43:
    case PerfEventType::Sample:
    case PerfEventType::TracePointSample:
        stream << event.m_origFrames << event.m_origNumGuessedFrames
               << static_cast<qint32>(PerfEvent::LastSpecialTypeId - event.typeIndex());
        if (feature != PerfEventType::Sample43) {
            stream << event.m_period << event.m_weight;
            if (feature == PerfEventType::TracePointSample)
                stream << event.m_traceData;
        }
        break;
    default:
        QTC_CHECK(false);
    }

    return stream;
}

} // namespace Internal
} // namespace PerfProfiler
