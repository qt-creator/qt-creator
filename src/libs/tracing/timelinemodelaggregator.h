// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

namespace Timeline {

class TRACING_EXPORT TimelineModelAggregator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QVariantList models READ models WRITE setModels NOTIFY modelsChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes WRITE setNotes NOTIFY notesChanged)
public:
    TimelineModelAggregator(QObject *parent = nullptr);
    ~TimelineModelAggregator() override;

    int height() const;
    int generateModelId();

    void addModel(TimelineModel *m);
    const TimelineModel *model(int modelIndex) const;

    QVariantList models() const;
    void setModels(const QVariantList &models);

    TimelineNotesModel *notes() const;
    void setNotes(TimelineNotesModel *notes);

    void clear();
    int modelCount() const;
    int modelIndexById(int modelId) const;

    Q_INVOKABLE int modelOffset(int modelIndex) const;

    Q_INVOKABLE QVariantMap nextItem(int selectedModel, int selectedItem, qint64 time) const;
    Q_INVOKABLE QVariantMap prevItem(int selectedModel, int selectedItem, qint64 time) const;

signals:
    void modelsChanged();
    void heightChanged();
    void notesChanged();
    void updateCursorPosition();

private:
    class TimelineModelAggregatorPrivate;
    TimelineModelAggregatorPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TimelineModelAggregator)
};

} // namespace Timeline
