// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
namespace Timeline {

class TimelineModel;
class TRACING_EXPORT TimelineNotesModel : public QObject
{
    Q_OBJECT

public:
    TimelineNotesModel(QObject *parent = nullptr);

    int count() const;
    void addTimelineModel(const TimelineModel *timelineModel);
    void removeTimelineModel(const TimelineModel *timelineModel);
    QList<const TimelineModel *> timelineModels() const;

    int typeId(int index) const;
    QString text(int index) const;
    int timelineModel(int index) const;
    int timelineIndex(int index) const;

    QList<int> byTypeId(int typeId) const;
    QList<int> byTimelineModel(int modelId) const;

    int get(int modelId, int timelineIndex) const;
    int add(int modelId, int timelineIndex, const QString &text);
    void update(int index, const QString &text);
    void remove(int index);

    void setText(int noteId, const QString &text);
    void setText(int modelId, int index, const QString &text);

    bool isModified() const;
    void resetModified();

    virtual void stash();
    virtual void restore();
    virtual void clear();

protected:
    const TimelineModel *timelineModelByModelId(int modelId) const;

signals:
    void changed(int typeId, int modelId, int timelineIndex);

private:
    struct Note {
        QString text;
        int timelineModel;
        int timelineIndex;
    };

    QList<Note> m_data;
    QHash<int, const TimelineModel *> m_timelineModels;
    bool m_modified = false;
};

} // namespace Timeline
