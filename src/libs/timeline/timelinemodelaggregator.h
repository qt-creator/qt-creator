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

#ifndef TIMELINEMODELAGGREGATOR_H
#define TIMELINEMODELAGGREGATOR_H

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

namespace Timeline {

class TIMELINE_EXPORT TimelineModelAggregator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes CONSTANT)
public:
    TimelineModelAggregator(TimelineNotesModel *notes, QObject *parent = 0);
    ~TimelineModelAggregator();

    int height() const;

    void addModel(TimelineModel *m);
    const TimelineModel *model(int modelIndex) const;
    QVariantList models() const;

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

#endif // TIMELINEMODELAGGREGATOR_H
