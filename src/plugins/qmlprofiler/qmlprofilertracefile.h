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

#include "qmleventlocation.h"
#include "qmlprofilereventtypes.h"
#include "qmlnote.h"
#include "qmleventtype.h"
#include "qmlevent.h"
#include "qmlprofilermodelmanager.h"

#include <tracing/timelinetracefile.h>

#include <QFutureInterface>
#include <QObject>
#include <QVector>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTraceFile : public Timeline::TimelineTraceFile
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceFile(QObject *parent = nullptr);

    void load(QIODevice *device) final;
    void save(QIODevice *device) final;

private:
    QmlProfilerModelManager *modelManager();
    QmlProfilerNotesModel *qmlNotes();
    void loadQtd(QIODevice *device);
    void loadQzt(QIODevice *device);
    void saveQtd(QIODevice *device);
    void saveQzt(QIODevice *device);

    void loadEventTypes(QXmlStreamReader &reader);
    void loadEvents(QXmlStreamReader &reader);
    void loadNotes(QXmlStreamReader &reader);

    enum ProgressValues {
        ProgressTypes  = 128,
        ProgressNotes  = 32,
        ProgressEvents = MaximumProgress - ProgressTypes - ProgressNotes - MinimumProgress,
    };

    void addEventsProgress(qint64 timestamp);
    void addStageProgress(ProgressValues stage);
};


} // namespace Internal
} // namespace QmlProfiler
