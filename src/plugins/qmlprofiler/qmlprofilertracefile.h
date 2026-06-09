// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"

#include <tracing/timelinetracefile.h>

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
QT_END_NAMESPACE

namespace Profiler::Internal {

class QmlProfilerNotesModel;

class QmlProfilerTraceFile : public Timeline::TimelineTraceFile
{
public:
    explicit QmlProfilerTraceFile(QObject *parent = nullptr);

    void load(QIODevice *device) final;
    void save(QIODevice *device) final;

private:
    QmlProfilerModelManager *modelManager();
    QmlProfilerNotesModel *qmlNotes();
    void loadQtd(QIODevice *device);
    void loadQzt(QIODevice *device);
    void saveQtd(QIODevice *device);
    void saveQzt(QIODevice *device);

    void loadEventTypes(QXmlStreamReader &reader);
    void loadEvents(QXmlStreamReader &reader);
    void loadNotes(QXmlStreamReader &reader);

    enum ProgressValues {
        ProgressTypes  = 128,
        ProgressNotes  = 32,
        ProgressEvents = MaximumProgress - ProgressTypes - ProgressNotes - MinimumProgress,
    };

    void addEventsProgress(qint64 timestamp);
    void addStageProgress(ProgressValues stage);
};

} // namespace Profiler::Internal
