// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include <QObject>
#include <QtQml/qqml.h>

namespace Timeline {

class TimelineModel;
class TRACING_EXPORT TimelineNotesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    QML_ANONYMOUS

public:
    TimelineNotesModel(QObject *parent = nullptr);
    ~TimelineNotesModel() override;

    int count() const;
    void addTimelineModel(const TimelineModel *timelineModel);
    void removeTimelineModel(const TimelineModel *timelineModel);
    QList<const TimelineModel *> timelineModels() const;

    Q_INVOKABLE int typeId(int index) const;
    Q_INVOKABLE QString text(int index) const;
    Q_INVOKABLE int timelineModel(int index) const;
    Q_INVOKABLE int timelineIndex(int index) const;

    Q_INVOKABLE QVariantList byTypeId(int typeId) const;
    Q_INVOKABLE QVariantList byTimelineModel(int modelId) const;

    Q_INVOKABLE int get(int modelId, int timelineIndex) const;
    Q_INVOKABLE int add(int modelId, int timelineIndex, const QString &text);
    Q_INVOKABLE void update(int index, const QString &text);
    Q_INVOKABLE void remove(int index);

    Q_INVOKABLE void setText(int noteId, const QString &text);
    Q_INVOKABLE void setText(int modelId, int index, const QString &text);

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
    class TimelineNotesModelPrivate;
    TimelineNotesModelPrivate *d_ptr;

    Q_DECLARE_PRIVATE(TimelineNotesModel)
};

} // namespace Timeline
