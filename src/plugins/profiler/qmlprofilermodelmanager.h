// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventlocation.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <tracing/timelinetracemanager.h>

#include <utils/fileinprojectfinder.h>

#include <functional>

namespace ProjectExplorer { class BuildConfiguration; }

namespace Profiler::Internal {

class QmlProfilerDetailsRewriter;

// Interface between the Data Model and the Engine/Tool
class QmlProfilerModelManager final : public Timeline::TimelineTraceManager
{
    Q_OBJECT
public:
    using QmlEventLoader
        = std::function<void (const QmlDebug::QmlEvent &, const QmlDebug::QmlEventType &)>;
    using QmlEventFilter = std::function<QmlEventLoader (QmlEventLoader)>;

    struct QmlLoaderEntry {
        quint64 features = 0;
        QmlEventLoader loader;
    };
    QList<QmlLoaderEntry> qmlLoaders;

    explicit QmlProfilerModelManager(QObject *parent = nullptr);

    const QmlDebug::QmlEventType &eventType(int typeId) const;

    void replayQmlEvents(QmlEventLoader loader, Initializer initializer, Finalizer finalizer,
                         ErrorHandler errorHandler, QFutureInterface<void> &future) const;

    void initialize() override;
    void finalize() override;

    void populateFileFinder(const ProjectExplorer::BuildConfiguration *bc = nullptr);
    Utils::FilePath findLocalFile(const QString &remoteFile);

    void useInMemoryEventStorage();

    static const char *featureName(QmlDebug::ProfileFeature feature);

    int appendEventType(QmlDebug::QmlEventType &&type);
    void setEventType(int typeId, QmlDebug::QmlEventType &&type);
    void appendEvent(QmlDebug::QmlEvent &&event);

    void restrictToRange(qint64 start, qint64 end);
    bool isRestrictedToRange() const;

    QmlEventFilter rangeFilter(qint64 start, qint64 end) const;

signals:
    void initialized();
    void typesCleared();
    void typeLocationAdded(int typeId, const QmlDebug::QmlEventLocation &location);
    void traceChanged();
    void typeDetailsChanged(int typeId);
    void typeDetailsFinished();

private:
    void loadEvent(const Timeline::TraceEvent &event,
                   const Timeline::TraceEventType &type) override;

    void setTypeDetails(int typeId, const QString &details);
    void restrictByFilter(QmlEventFilter filter);

    void clearEventStorage() final;
    void clearTypeStorage() final;

    Timeline::TimelineTraceFile *createTraceFile() override;
    void replayEvents(TraceEventLoader loader, Initializer initializer, Finalizer finalizer,
                      ErrorHandler errorHandler, QFutureInterface<void> &future) const override;

    QmlProfilerDetailsRewriter *m_detailsRewriter = nullptr;
    bool m_isRestrictedToRange = false;
};

} // namespace Profiler::Internal
