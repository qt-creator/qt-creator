// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilertracemanager.h"

#include <tracing/timelinetracefile.h>

#include <QDataStream>
#include <QObject>
#include <QPointer>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerTraceFile : public Timeline::TimelineTraceFile
{
    Q_OBJECT
public:
    explicit PerfProfilerTraceFile(QObject *parent = nullptr);

    void load(QIODevice *device) override;
    void save(QIODevice *device) override;

signals:
    void messagesAvailable(const QByteArray &buffer);
    void blockAvailable(const QByteArray &buffer);

protected:

    void clear();
    void setDevice(QIODevice *device) { m_device = device; }
    void readFromDevice();
    void writeToDevice();
    void readBlock(const QByteArray &block);
    void readMessages(const QByteArray &buffer);
    bool updateProgress(int progress);

    PerfProfilerTraceManager *manager()
    {
        return static_cast<PerfProfilerTraceManager *>(traceManager());
    }

    virtual qint64 adjustTimestamp(qint64 timestamp);
    virtual bool acceptsSamples() const;

    QPointer<QIODevice> m_device;
    quint32 m_messageSize;
    qint32 m_dataStreamVersion;
    bool m_compressed;
};

} // namespace Internal
} // namespace PerfProfiler
