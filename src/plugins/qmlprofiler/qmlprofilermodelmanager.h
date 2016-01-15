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

#ifndef QMLPROFILERMODELMANAGER_H
#define QMLPROFILERMODELMANAGER_H

#include "qmlprofiler_global.h"

#include <qmldebug/qmlprofilereventlocation.h>
#include <qmldebug/qmlprofilereventtypes.h>
#include <utils/fileinprojectfinder.h>

#include <QObject>

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
    ~QmlProfilerTraceTime();

    qint64 startTime() const;
    qint64 endTime() const;
    qint64 duration() const;

signals:
    void timeChanged(qint64,qint64);

public slots:
    void clear();

    void setTime(qint64 startTime, qint64 endTime);
    void decreaseStartTime(qint64 time);
    void increaseEndTime(qint64 time);

private:
    qint64 m_startTime;
    qint64 m_endTime;
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

    explicit QmlProfilerModelManager(Utils::FileInProjectFinder *finder, QObject *parent = 0);
    ~QmlProfilerModelManager();

    State state() const;
    QmlProfilerTraceTime *traceTime() const;
    QmlProfilerDataModel *qmlModel() const;
    QmlProfilerNotesModel *notesModel() const;

    bool isEmpty() const;

    double progress() const;
    int registerModelProxy();
    void setProxyCountWeight(int proxyId, int weight);
    void modelProxyCountUpdated(int proxyId, qint64 count, qint64 max);
    void announceFeatures(int proxyId, quint64 features);
    quint64 availableFeatures() const;
    quint64 visibleFeatures() const;
    void setVisibleFeatures(quint64 features);
    quint64 recordedFeatures() const;
    void setRecordedFeatures(quint64 features);

    void acquiringDone();
    void processingDone();

    static const char *featureName(QmlDebug::ProfileFeature feature);

signals:
    void error(const QString &error);
    void stateChanged();
    void progressChanged();
    void loadFinished();
    void saveFinished();

    void requestDetailsForLocation(int eventType, const QmlDebug::QmlEventLocation &location);
    void availableFeaturesChanged(quint64 features);
    void visibleFeaturesChanged(quint64 features);
    void recordedFeaturesChanged(quint64 features);

public slots:
    void clear();

    void prepareForWriting();
    void addQmlEvent(QmlDebug::Message message, QmlDebug::RangeType rangeType, int bindingType,
                     qint64 startTime, qint64 length, const QString &data,
                     const QmlDebug::QmlEventLocation &location,
                     qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5);
    void addDebugMessage(QtMsgType type, qint64 timestamp, const QString &text,
                         const QmlDebug::QmlEventLocation &location);

    void save(const QString &filename);
    void load(const QString &filename);

private:
    void setState(State state);

private:
    class QmlProfilerModelManagerPrivate;
    QmlProfilerModelManagerPrivate *d;
};

}

#endif
