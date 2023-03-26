// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QFutureInterface>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace Timeline {

class TimelineNotesModel;
class TimelineTraceManager;

class TRACING_EXPORT TimelineTraceFile : public QObject
{
    Q_OBJECT
public:
    enum ProgressValues {
        MinimumProgress = 0,
        MaximumProgress = 1000
    };

    explicit TimelineTraceFile(QObject *parent = nullptr);

    void setTraceManager(TimelineTraceManager *traceManager) { m_traceManager = traceManager; }
    TimelineTraceManager *traceManager() const { return m_traceManager; }

    void setNotes(TimelineNotesModel *notes) { m_notes = notes; }
    TimelineNotesModel *notes() const { return m_notes; }

    void setTraceTime(qint64 startTime, qint64 endTime, qint64 measturedTime);
    void setFuture(const QFutureInterface<void> &future);
    QFutureInterface<void> &future();

    bool isProgressUpdateNeeded() const;
    void addProgressValue(int progressValue);
    void setDeviceProgress(QIODevice *device);

    virtual void save(QIODevice *device) = 0;
    virtual void load(QIODevice *device) = 0;

    void fail(const QString &message);
    void finish();

    void setTraceStart(qint64 traceStart) { m_traceStart = traceStart; }
    qint64 traceStart() const { return m_traceStart; }

    void setTraceEnd(qint64 traceEnd) { m_traceEnd = traceEnd; }
    qint64 traceEnd() const { return m_traceEnd; }
    qint64 measuredTime() const { return m_measuredTime; }

    bool isCanceled() const;

    quint64 loadedFeatures() const { return m_loadedFeatures; }
    void addFeature(quint8 feature) { m_loadedFeatures |= (1ull << feature); }

signals:
    void error(const QString &error);

private:
    qint64 m_traceStart = -1;
    qint64 m_traceEnd = -1;
    qint64 m_measuredTime = -1;
    quint64 m_loadedFeatures = 0;

    QFutureInterface<void> m_future;

    TimelineTraceManager *m_traceManager = nullptr;
    TimelineNotesModel *m_notes = nullptr;
};

} // namespace Timeline
