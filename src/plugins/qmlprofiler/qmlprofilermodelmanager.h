// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"
#include "qmlprofilertextmark.h"

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventlocation.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/fileinprojectfinder.h>
#include <tracing/timelinetracemanager.h>

#include <QObject>
#include <functional>

namespace ProjectExplorer { class BuildConfiguration; }

namespace QmlProfiler {

class QmlProfilerModelManager;
class QmlProfilerNotesModel;

// Interface between the Data Model and the Engine/Tool
class QMLPROFILER_EXPORT QmlProfilerModelManager : public Timeline::TimelineTraceManager
{
    Q_OBJECT
public:
    using QmlEventLoader
        = std::function<void (const QmlDebug::QmlEvent &, const QmlDebug::QmlEventType &)>;
    using QmlEventFilter = std::function<QmlEventLoader (QmlEventLoader)>;

    explicit QmlProfilerModelManager(QObject *parent = nullptr);
    ~QmlProfilerModelManager() override;

    Internal::QmlProfilerTextMarkModel *textMarkModel() const;

    void registerFeatures(quint64 features, QmlEventLoader eventLoader,
                          Initializer initializer = nullptr, Finalizer finalizer = nullptr,
                          Clearer clearer = nullptr);

    const QmlDebug::QmlEventType &eventType(int typeId) const;

    void replayQmlEvents(QmlEventLoader loader, Initializer initializer, Finalizer finalizer,
                         ErrorHandler errorHandler, QFutureInterface<void> &future) const;

    void initialize() override;
    void finalize() override;

    void populateFileFinder(const ProjectExplorer::BuildConfiguration *bc = nullptr);
    Utils::FilePath findLocalFile(const QString &remoteFile);

    static const char *featureName(QmlDebug::ProfileFeature feature);

    int appendEventType(QmlDebug::QmlEventType &&type);
    void setEventType(int typeId, QmlDebug::QmlEventType &&type);
    void appendEvent(QmlDebug::QmlEvent &&event);

    void restrictToRange(qint64 start, qint64 end);
    bool isRestrictedToRange() const;

    QmlEventFilter rangeFilter(qint64 start, qint64 end) const;

signals:
    void traceChanged();
    void typeDetailsChanged(int typeId);
    void typeDetailsFinished();

private:
    void setTypeDetails(int typeId, const QString &details);
    void restrictByFilter(QmlEventFilter filter);

    void clearEventStorage() final;
    void clearTypeStorage() final;

    Timeline::TimelineTraceFile *createTraceFile() override;
    void replayEvents(TraceEventLoader loader, Initializer initializer, Finalizer finalizer,
                      ErrorHandler errorHandler, QFutureInterface<void> &future) const override;

    class QmlProfilerModelManagerPrivate;
    QmlProfilerModelManagerPrivate *d;
};

} // namespace QmlProfiler
