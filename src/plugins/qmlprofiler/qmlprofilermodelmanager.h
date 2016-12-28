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

#include "qmlprofiler_global.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlevent.h"
#include "qmleventtype.h"

#include <utils/fileinprojectfinder.h>

#include <QObject>
#include <functional>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerDataModel;
class QmlProfilerNotesModel;

namespace Internal {

class QMLPROFILER_EXPORT QmlProfilerTraceTime : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerTraceTime(QObject *parent);

    qint64 startTime() const;
    qint64 endTime() const;
    qint64 duration() const;
    bool isRestrictedToRange() const;

public slots:
    void clear();

    void setTime(qint64 startTime, qint64 endTime);
    void decreaseStartTime(qint64 time);
    void increaseEndTime(qint64 time);
    void restrictToRange(qint64 startTime, qint64 endTime);

private:
    qint64 m_startTime;
    qint64 m_endTime;

    qint64 m_restrictedStartTime;
    qint64 m_restrictedEndTime;
};

} // End internal namespace

using namespace Internal;

// Interface between the Data Model and the Engine/Tool
class QMLPROFILER_EXPORT QmlProfilerModelManager : public QObject
{
    Q_OBJECT
public:
    enum State {
        Empty,
        AcquiringData,
        ProcessingData,
        ClearingData,
        Done
    };

    typedef std::function<void(const QmlEvent &, const QmlEventType &)> EventLoader;
    typedef std::function<void()> Finalizer;

    explicit QmlProfilerModelManager(QObject *parent = 0);
    ~QmlProfilerModelManager();

    State state() const;
    QmlProfilerTraceTime *traceTime() const;
    QmlProfilerDataModel *qmlModel() const;
    QmlProfilerNotesModel *notesModel() const;

    bool isEmpty() const;
    uint numLoadedEvents() const;
    uint numLoadedEventTypes() const;

    int registerModelProxy();
    void announceFeatures(quint64 features, EventLoader eventLoader, Finalizer finalizer);

    int numFinishedFinalizers() const;
    int numRegisteredFinalizers() const;

    void addEvents(const QVector<QmlEvent> &events);
    void addEvent(const QmlEvent &event);

    void addEventTypes(const QVector<QmlEventType> &types);
    void addEventType(const QmlEventType &type);

    quint64 availableFeatures() const;
    quint64 visibleFeatures() const;
    void setVisibleFeatures(quint64 features);
    quint64 recordedFeatures() const;
    void setRecordedFeatures(quint64 features);
    bool aggregateTraces() const;
    void setAggregateTraces(bool aggregateTraces);

    void acquiringDone();
    void processingDone();

    static const char *featureName(ProfileFeature feature);

signals:
    void error(const QString &error);
    void stateChanged();
    void loadFinished();
    void saveFinished();

    void availableFeaturesChanged(quint64 features);
    void visibleFeaturesChanged(quint64 features);
    void recordedFeaturesChanged(quint64 features);

public slots:
    void clear();
    void restrictToRange(qint64 startTime, qint64 endTime);
    bool isRestrictedToRange() const;

    void startAcquiring();

    void save(const QString &filename);
    void load(const QString &filename);

private:
    void setState(State state);

private:
    class QmlProfilerModelManagerPrivate;
    QmlProfilerModelManagerPrivate *d;
};

} // namespace QmlProfiler
