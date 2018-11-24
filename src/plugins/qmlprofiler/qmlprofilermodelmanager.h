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
    QString findLocalFile(const QString &remoteFile);

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
