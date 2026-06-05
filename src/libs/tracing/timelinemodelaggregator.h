// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

namespace Timeline {

struct ModelItem
{
    int modelIndex = -1;
    int itemIndex = -1;
};

class TRACING_EXPORT TimelineModelAggregator : public QObject
{
    Q_OBJECT

public:
    TimelineModelAggregator(QObject *parent = nullptr);
    ~TimelineModelAggregator() override;

    int height() const;
    int generateModelId();

    void addModel(TimelineModel *m); // Not owned
    const TimelineModel *model(int modelIndex) const;

    QList<TimelineModel *> models() const;
    void setModels(const QList<TimelineModel *> &models);

    TimelineNotesModel *notes() const;
    void setNotes(TimelineNotesModel *notes);

    void clear();
    int modelCount() const;
    int modelIndexById(int modelId) const;

    int modelOffset(int modelIndex) const;

    void moveModel(int from, int to);

    ModelItem nextItem(int selectedModel, int selectedItem, qint64 time) const;
    ModelItem prevItem(int selectedModel, int selectedItem, qint64 time) const;

signals:
    void modelsChanged();
    void heightChanged();
    void notesChanged();
    void updateCursorPosition();

private:
    class TimelineModelAggregatorPrivate *d;
};

} // namespace Timeline
