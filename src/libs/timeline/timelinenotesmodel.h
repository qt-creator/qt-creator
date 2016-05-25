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

namespace Timeline {

class TIMELINE_EXPORT TimelineNotesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
public:
    TimelineNotesModel(QObject *parent = 0);
    ~TimelineNotesModel();

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

    void clear();

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
