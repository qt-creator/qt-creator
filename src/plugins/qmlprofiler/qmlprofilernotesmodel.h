// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"
#include "qmlnote.h"

#include <tracing/timelinenotesmodel.h>

#include <QList>
#include <QHash>

namespace QmlProfiler {
class QMLPROFILER_EXPORT QmlProfilerNotesModel : public Timeline::TimelineNotesModel {
    Q_OBJECT
public:
    QmlProfilerNotesModel(QObject *parent);

    void restore() override;
    void stash() override;

    const QVector<QmlNote> &notes() const;
    void setNotes(const QVector<QmlNote> &notes);
    void addNote(const QmlNote &note);
    void clear() override;

protected:
    QVector<QmlNote> m_notes;

    int addQmlNote(int typeId, int collapsedRow, qint64 startTime, qint64 duration,
                   const QString &text);
};
} // namespace QmlProfiler
