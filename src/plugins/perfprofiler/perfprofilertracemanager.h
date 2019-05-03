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

#include "perfevent.h"
#include "perfeventtype.h"

#include <projectexplorer/kit.h>

#include <tracing/timelinetracemanager.h>
#include <tracing/tracestashfile.h>

#include <QTimer>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerEventStorage : public Timeline::TraceEventStorage
{
    Q_DECLARE_TR_FUNCTIONS(QmlProfilerEventStorage)
public:
    using StashFile = Timeline::TraceStashFile<PerfEvent>;

    PerfProfilerEventStorage(const std::function<void(const QString &)> &errorHandler);

    int append(Timeline::TraceEvent &&event) override;
    int size() const override;
    void clear() override;
    void finalize() override;
    bool replay(const std::function<bool (Timeline::TraceEvent &&)> &receiver) const override;

    StashFile::Iterator iterator() const { return m_file.iterator(); }

private:
    StashFile m_file;
    std::function<void(const QString &)> m_errorHandler;
    int m_size = 0;
};

class PerfProfilerTraceManager : public Timeline::TimelineTraceManager
{
    Q_OBJECT
    Q_PROPERTY(bool aggregateAddresses READ aggregateAddresses WRITE setAggregateAddresses
               NOTIFY aggregateAddressesChanged)
public:
    typedef std::function<void(const PerfEvent &, const PerfEventType &)> PerfEventLoader;
    typedef std::function<PerfEventLoader(PerfEventLoader)> PerfEventFilter;

    struct Thread {
        Thread(qint64 start = -1, qint64 firstEvent = -1, qint64 lastEvent = -1, quint32 pid = 0,
               quint32 tid = 0, quint32 cpu = 0, qint32 name = -1, bool enabled = false) :
            start(start), firstEvent(firstEvent), lastEvent(lastEvent), pid(pid), tid(tid),
            cpu(cpu), name(name), enabled(enabled)
        {}

        qint64 start;
        qint64 firstEvent;
        qint64 lastEvent;
        quint32 pid;
        quint32 tid;
        quint32 cpu;
        qint32 name;
        bool enabled;
    };

    struct Symbol {
        Symbol() : name(-1), binary(-1), path(-1), isKernel(false) {}

        qint32 name;
        qint32 binary;
        qint32 path;
        bool isKernel;
    };

    struct TracePoint {
        TracePoint() : system(-1), name(-1), flags(0) {}

        qint32 system;
        qint32 name;
        quint32 flags;
    };

    static const QByteArray s_resourceNamePrefix;
    static const QByteArray s_resourceReleasedIdName;
    static const QByteArray s_resourceRequestedBlocksName;
    static const QByteArray s_resourceRequestedAmountName;
    static const QByteArray s_resourceObtainedIdName;
    static const QByteArray s_resourceMovedIdName;

    explicit PerfProfilerTraceManager(QObject *parent = nullptr);
    ~PerfProfilerTraceManager() override;

    void registerFeatures(quint64 features, PerfEventLoader eventLoader,
                          Initializer intializer = nullptr,
                          Finalizer finalizer = nullptr,
                          Clearer clearer = nullptr);

    void replayPerfEvents(PerfEventLoader loader, Initializer initializer, Finalizer finalizer,
                          ErrorHandler errorHandler, QFutureInterface<void> &future) const;

    bool open();

    void setThread(quint32 tid, const Thread &thread);
    void setThreadEnabled(quint32 tid, bool enabled);
    const Thread &thread(quint32 tid) const;

    const PerfEventType &eventType(int id) const;
    void setEventType(int id, PerfEventType &&type);

    void appendEvent(PerfEvent &&event);
    void appendSampleEvent(PerfEvent &&event);

    void setString(qint32 id, const QByteArray &value);
    void setSymbol(qint32 id, const Symbol &symbol);
    void setTracePoint(qint32 id, const TracePoint &tracePoint);

    const QByteArray &string(qint32 id) const;
    const Symbol &symbol(qint32 id) const;
    const TracePoint &tracePoint(qint32 id) const;

    const PerfEventType::Location &location(qint32 id) const;
    const PerfEventType::Attribute &attribute(qint32 id) const;

    qint32 symbolLocation(qint32 locationId) const;

    void setAggregateAddresses(bool aggregateAddresses);
    bool aggregateAddresses() const { return m_aggregateAddresses; }

    qint32 resourceReleasedIdId() const { return m_resourceReleasedIdId; }
    qint32 resourceRequestedAmountId() const { return m_resourceRequestedAmountId; }
    qint32 resourceRequestedBlocksId() const { return m_resourceRequestedBlocksId; }
    qint32 resourceObtainedIdId() const { return m_resourceObtainedIdId; }
    qint32 resourceMovedIdId() const { return m_resourceMovedIdId; }

    const std::vector<PerfEventType> &attributes() const;
    const std::vector<PerfEventType> &locations() const;
    const QVector<QByteArray> &strings() const { return m_strings; }
    const QHash<qint32, Symbol> &symbols() const { return m_symbols; }
    const QHash<qint32, TracePoint> &tracePoints() const { return m_tracePoints; }
    const QHash<quint32, Thread> &threads() const { return m_threads; }

    void loadFromTraceFile(const QString &filePath);
    void saveToTraceFile(const QString &filePath);
    void loadFromPerfData(const QString &filePath, const QString &executableDirPath,
                          ProjectExplorer::Kit *kit);

    void finalize() override;

    qint64 samplingFrequency() const;

    PerfEventFilter rangeAndThreadFilter(qint64 rangeStart, qint64 rangeEnd) const;

    void restrictByFilter(PerfEventFilter filter);

signals:
    void aggregateAddressesChanged(bool aggregateAddresses);
    void threadEnabledChanged(quint32 tid, bool enabled);

protected:
    void clearTypeStorage() override;

    Timeline::TimelineTraceFile *createTraceFile() override;
    void replayEvents(TraceEventLoader loader, Initializer initializer, Finalizer finalizer,
                      ErrorHandler errorHandler, QFutureInterface<void> &future) const override;

private:
    QTimer m_reparseTimer;

    QVector<QByteArray> m_strings;
    QHash<qint32, Symbol> m_symbols;
    QHash<qint32, TracePoint> m_tracePoints;
    QHash<quint32, Thread> m_threads;

    struct ViolatedStorage
    {
        using Iterator = PerfProfilerEventStorage::StashFile::Iterator;
        std::unique_ptr<Timeline::TraceEventStorage> storage;
        Iterator iterator;
    };

    std::vector<ViolatedStorage> m_violatedStorages;

    bool m_aggregateAddresses = false;

    qint32 m_resourceReleasedIdId = -1;
    qint32 m_resourceRequestedAmountId = -1;
    qint32 m_resourceObtainedIdId = -1;
    qint32 m_resourceMovedIdId = -1;
    qint32 m_resourceRequestedBlocksId = -1;

    void resetAttributes();
    void processSample(PerfEvent &event) const;
    void checkThread(const PerfEvent &event);
    void handleTimeOrderViolations(qint64 nextEvent);
};

inline QDataStream &operator>>(QDataStream &stream, PerfProfilerTraceManager::Symbol &symbol)
{
    return stream >> symbol.name >> symbol.binary >> symbol.path >> symbol.isKernel;
}

inline QDataStream &operator<<(QDataStream &stream, const PerfProfilerTraceManager::Symbol &symbol)
{
    return stream << symbol.name << symbol.binary << symbol.path << symbol.isKernel;
}

inline QDataStream &operator>>(QDataStream &stream,
                               PerfProfilerTraceManager::TracePoint &tracePoint)
{
    return stream >> tracePoint.system >> tracePoint.name >> tracePoint.flags;
}

inline QDataStream &operator<<(QDataStream &stream,
                               const PerfProfilerTraceManager::TracePoint &tracePoint)
{
    return stream << tracePoint.system << tracePoint.name << tracePoint.flags;
}

inline QDataStream &operator>>(QDataStream &stream, PerfProfilerTraceManager::Thread &thread)
{
    stream >> thread.pid >> thread.tid >> thread.start >> thread.cpu >> thread.name;
    thread.enabled = (thread.pid != 0);
    return stream;
}

inline QDataStream &operator<<(QDataStream &stream, const PerfProfilerTraceManager::Thread &thread)
{
    return stream << thread.pid << thread.tid << thread.start << thread.cpu << thread.name;
}


} // namespace Internal
} // namespace PerfProfiler
