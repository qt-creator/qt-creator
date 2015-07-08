/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

class QmlProfilerDataState : public QObject
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

    explicit QmlProfilerDataState(QmlProfilerModelManager *modelManager, QObject *parent = 0);
    ~QmlProfilerDataState() {}

    State state() const { return m_state; }

signals:
    void stateChanged();
    void error(const QString &error);

private:
    void setState(State state);
    State m_state;
    QmlProfilerModelManager *m_modelManager;

    friend class QmlProfiler::QmlProfilerModelManager;
};

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

    explicit QmlProfilerModelManager(Utils::FileInProjectFinder *finder, QObject *parent = 0);
    ~QmlProfilerModelManager();

    QmlProfilerDataState::State state() const;
    QmlProfilerTraceTime *traceTime() const;
    QmlProfilerDataModel *qmlModel() const;
    QmlProfilerNotesModel *notesModel() const;

    bool isEmpty() const;
    int count() const;

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

    void complete();

    void save(const QString &filename);
    void load(const QString &filename);
    void setFilename(const QString &filename);
    void load();

    void newTimeEstimation(qint64 estimation);

private:
    void setState(QmlProfilerDataState::State state);


private:
    class QmlProfilerModelManagerPrivate;
    QmlProfilerModelManagerPrivate *d;
};

}

#endif
