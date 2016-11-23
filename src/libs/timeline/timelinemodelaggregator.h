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

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

namespace Timeline {

class TIMELINE_EXPORT TimelineModelAggregator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QVariantList models READ models WRITE setModels NOTIFY modelsChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes CONSTANT)
public:
    TimelineModelAggregator(TimelineNotesModel *notes, QObject *parent = 0);
    ~TimelineModelAggregator();

    int height() const;

    void addModel(TimelineModel *m);
    const TimelineModel *model(int modelIndex) const;

    QVariantList models() const;
    void setModels(const QVariantList &models);

    TimelineNotesModel *notes() const;
    void clear();
    int modelCount() const;
    int modelIndexById(int modelId) const;

    Q_INVOKABLE int modelOffset(int modelIndex) const;

    Q_INVOKABLE QVariantMap nextItem(int selectedModel, int selectedItem, qint64 time) const;
    Q_INVOKABLE QVariantMap prevItem(int selectedModel, int selectedItem, qint64 time) const;

signals:
    void modelsChanged();
    void heightChanged();

private:
    class TimelineModelAggregatorPrivate;
    TimelineModelAggregatorPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TimelineModelAggregator)
};

} // namespace Timeline
