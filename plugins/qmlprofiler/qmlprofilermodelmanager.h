/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QObject>
#include "qmlprofiler_global.h"
#include "qmldebug/qmlprofilereventlocation.h"
#include <utils/fileinprojectfinder.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerSimpleModel;
class QV8ProfilerDataModel;
class QmlProfilerModelManager;
class QmlProfilerDataState : public QObject
{
    Q_OBJECT
public:
    enum State {
        Empty,
        AcquiringData,
        ProcessingData,
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

    friend class QmlProfilerModelManager;
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
    void addRangedEvent(int type, int bindingType, qint64 startTime, qint64 length,
                        const QStringList &data, const QmlDebug::QmlEventLocation &location);
    void addSceneGraphEvent(int eventType, int SGEtype, qint64 startTime,
                            qint64 timing1, qint64 timing2, qint64 timing3, qint64 timing4, qint64 timing5);
    void addPixmapCacheEvent(qint64 time, int pixmapEventType, QString Url,
                             int pixmapWidth, int pixmapHeight, int referenceCount);
    void addV8Event(int depth, const QString &function,const QString &filename, int lineNumber,
                    double totalTime, double selfTime);

    void addFrameEvent(qint64 time, int framerate, int animationcount);

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
}

#endif
