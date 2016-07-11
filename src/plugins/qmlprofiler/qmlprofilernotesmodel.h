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

#include "qmlprofilermodelmanager.h"
#include "qmlnote.h"
#include "timeline/timelinenotesmodel.h"
#include <QList>
#include <QHash>

namespace QmlProfiler {
class QMLPROFILER_EXPORT QmlProfilerNotesModel : public Timeline::TimelineNotesModel {
    Q_OBJECT
public:
    QmlProfilerNotesModel(QObject *parent);

    void loadData();
    void saveData();

    const QVector<QmlNote> &notes() const;
    void setNotes(const QVector<QmlNote> &notes);
    void clear();

protected:
    QVector<QmlNote> m_notes;

    int addQmlNote(int typeId, int collapsedRow, qint64 startTime, qint64 duration,
                   const QString &text);
};
} // namespace QmlProfiler
