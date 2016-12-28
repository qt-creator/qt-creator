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
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmleventtype.h"
#include "qmlevent.h"

#include <utils/fileinprojectfinder.h>
#include <projectexplorer/runconfiguration.h>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerDataModel : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerDataModel(QObject *parent = nullptr);
    ~QmlProfilerDataModel();

    const QmlEventType &eventType(int typeId) const;
    const QVector<QmlEventType> &eventTypes() const;
    void addEventTypes(const QVector<QmlEventType> &types);
    void addEventType(const QmlEventType &type);

    void populateFileFinder(const ProjectExplorer::RunConfiguration *runConfiguration = nullptr);
    QString findLocalFile(const QString &remoteFile);

    void clear();
    bool isEmpty() const;
    void addEvent(const QmlEvent &event);
    void addEvents(const QVector<QmlEvent> &events);
    void replayEvents(qint64 startTime, qint64 endTime,
                      QmlProfilerModelManager::EventLoader loader) const;
    void finalize();

signals:
    void allTypesLoaded();

protected slots:
    void detailsChanged(int typeId, const QString &newString);

private:
    class QmlProfilerDataModelPrivate;
    QmlProfilerDataModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QmlProfilerDataModel)
};

} // namespace QmlProfiler
