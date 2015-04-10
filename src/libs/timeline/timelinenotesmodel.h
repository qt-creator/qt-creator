/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINENOTESMODEL_H
#define TIMELINENOTESMODEL_H

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
    Q_PRIVATE_SLOT(d_ptr, void _q_removeTimelineModel(QObject *timelineModel))
};

} // namespace Timeline

#endif // TIMELINENOTESMODEL_H
