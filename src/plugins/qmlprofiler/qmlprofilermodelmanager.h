// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlevent.h"
#include "qmleventtype.h"
#include "qmlprofilertextmark.h"

#include <utils/fileinprojectfinder.h>
#include <tracing/timelinetracemanager.h>

#include <QObject>
#include <functional>

namespace ProjectExplorer { class Target; }

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerNotesModel;

// Interface between the Data Model and the Engine/Tool
class QMLPROFILER_EXPORT QmlProfilerModelManager : public Timeline::TimelineTraceManager
{
    Q_OBJECT
public:
    using QmlEventLoader = std::function<void (const QmlEvent &, const QmlEventType &)>;
    using QmlEventFilter = std::function<QmlEventLoader (QmlEventLoader)>;

    explicit QmlProfilerModelManager(QObject *parent = nullptr);
    ~QmlProfilerModelManager() override;

    Internal::QmlProfilerTextMarkModel *textMarkModel() const;

    void registerFeatures(quint64 features, QmlEventLoader eventLoader,
                          Initializer initializer = nullptr, Finalizer finalizer = nullptr,
                          Clearer clearer = nullptr);

    const QmlEventType &eventType(int typeId) const;

    void replayQmlEvents(QmlEventLoader loader, Initializer initializer, Finalizer finalizer,
                         ErrorHandler errorHandler, QFutureInterface<void> &future) const;

    void initialize() override;
    void finalize() override;

    void populateFileFinder(const ProjectExplorer::Target *target = nullptr);
    Utils::FilePath findLocalFile(const QString &remoteFile);

    static const char *featureName(ProfileFeature feature);

    int appendEventType(QmlEventType &&type);
    void setEventType(int typeId, QmlEventType &&type);
    void appendEvent(QmlEvent &&event);

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
