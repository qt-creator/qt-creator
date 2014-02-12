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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROFILERMODELMANAGER_H
#define QMLPROFILERMODELMANAGER_H

#include "qmlprofiler_global.h"

#include <qmldebug/qmlprofilereventlocation.h>
#include <utils/fileinprojectfinder.h>

#include <QObject>

namespace QmlProfiler {
class QmlProfilerSimpleModel;
class QmlProfilerModelManager;

namespace Internal {

class QV8ProfilerDataModel;
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

class QmlProfilerTraceTime : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerTraceTime(QObject *parent);
    ~QmlProfilerTraceTime();

    qint64 startTime() const;
    qint64 endTime() const;
    qint64 duration() const;

public slots:
    void clear();
    void setStartTime(qint64 time);
    void setEndTime(qint64 time);

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
    QmlProfilerSimpleModel *simpleModel() const;
    QV8ProfilerDataModel *v8Model() const;

    bool isEmpty() const;
    int count() const;

    double progress() const;
    int registerModelProxy();
    void setProxyCountWeight(int proxyId, int weight);
    void modelProxyCountUpdated(int proxyId, qint64 count, qint64 max);

    qint64 estimatedProfilingTime() const;

signals:
    void countChanged();
    void error(const QString &error);
    void stateChanged();
    void progressChanged();
    void dataAvailable();

    void requestDetailsForLocation(int eventType, const QmlDebug::QmlEventLocation &location);

public slots:
    void clear();

    void prepareForWriting();
    void addQmlEvent(int type, int bindingType, qint64 startTime, qint64 length,
                        const QStringList &data, const QmlDebug::QmlEventLocation &location,
                     qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5);
    void addV8Event(int depth, const QString &function,const QString &filename, int lineNumber,
                    double totalTime, double selfTime);

    void complete();
    void modelProcessingDone();

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
