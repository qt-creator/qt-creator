// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlnote.h"

#include <tracing/timelinenotesmodel.h>

#include <QList>
#include <QHash>

namespace QmlProfiler::Internal {

class QmlProfilerNotesModel final : public Timeline::TimelineNotesModel
{
public:
    QmlProfilerNotesModel(QObject *parent);

    void restore() final;
    void stash() final;
    void clear() final;

    const QList<QmlNote> &notes() const;
    void setNotes(const QList<QmlNote> &notes);
    void addNote(const QmlNote &note);

protected:
    QList<QmlNote> m_notes;

    int addQmlNote(int typeId, int collapsedRow, qint64 startTime, qint64 duration,
                   const QString &text);
};

} // namespace QmlProfiler::Internal
