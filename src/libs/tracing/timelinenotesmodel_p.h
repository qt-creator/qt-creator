// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinenotesmodel.h"

namespace Timeline {

class TimelineNotesModel::TimelineNotesModelPrivate {
public:
    TimelineNotesModelPrivate(TimelineNotesModel *q);

    struct Note {
        // Saved properties
        QString text;

        // Cache, created on loading
        int timelineModel;
        int timelineIndex;
    };

    QList<Note> data;
    QHash<int, const TimelineModel *> timelineModels;
    bool modified;

private:
    TimelineNotesModel *q_ptr;
    Q_DECLARE_PUBLIC(TimelineNotesModel)
};

} // namespace Timeline
