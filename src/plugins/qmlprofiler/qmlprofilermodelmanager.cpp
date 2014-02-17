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

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "qv8profilerdatamodel.h"
#include "qmlprofilertracefile.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>

namespace QmlProfiler {
namespace Internal {


/////////////////////////////////////////////////////////////////////
QmlProfilerDataState::QmlProfilerDataState(QmlProfilerModelManager *modelManager, QObject *parent)
    : QObject(parent), m_state(Empty), m_modelManager(modelManager)
{
    connect(this, SIGNAL(error(QString)), m_modelManager, SIGNAL(error(QString)));
    connect(this, SIGNAL(stateChanged()), m_modelManager, SIGNAL(stateChanged()));
}

void QmlProfilerDataState::setState(QmlProfilerDataState::State state)
{
    // It's not an error, we are continuously calling "AcquiringData" for example
    if (m_state == state)
        return;

    switch (state) {
        case ClearingData:
            QTC_ASSERT(m_state == Done || m_state == Empty || m_state == AcquiringData, /**/);
        break;
        case Empty:
            // if it's not empty, complain but go on
            QTC_ASSERT(m_modelManager->isEmpty(), /**/);
        break;
        case AcquiringData:
            // we're not supposed to receive new data while processing older data
            QTC_ASSERT(m_state != ProcessingData, return);
        break;
        case ProcessingData:
            QTC_ASSERT(m_state == AcquiringData, return);
        break;
        case Done:
            QTC_ASSERT(m_state == ProcessingData || m_state == Empty, return);
        break;
        default:
            emit error(tr("Trying to set unknown state in events list."));
        break;
    }

    m_state = state;
    emit stateChanged();

    return;
}


/////////////////////////////////////////////////////////////////////
QmlProfilerTraceTime::QmlProfilerTraceTime(QObject *parent) : QObject(parent)
{
    clear();
}

QmlProfilerTraceTime::~QmlProfilerTraceTime()
{
}

qint64 QmlProfilerTraceTime::startTime() const
{
    return m_startTime;
}

qint64 QmlProfilerTraceTime::endTime() const
{
    return m_endTime;
}

qint64 QmlProfilerTraceTime::duration() const
{
    return endTime() - startTime();
}

void QmlProfilerTraceTime::clear()
{
    m_startTime = -1;
    m_endTime = 0;
}

void QmlProfilerTraceTime::setStartTime(qint64 time)
{
    m_startTime = time;
}

void QmlProfilerTraceTime::setEndTime(qint64 time)
{
    m_endTime = time;
}


} // namespace Internal

/////////////////////////////////////////////////////////////////////

class QmlProfilerModelManager::QmlProfilerModelManagerPrivate
{
public:
    QmlProfilerModelManagerPrivate(QmlProfilerModelManager *qq) : q(qq) {}
    ~QmlProfilerModelManagerPrivate() {}
    QmlProfilerModelManager *q;

    QmlProfilerDataModel *model;
    QV8ProfilerDataModel *v8Model;
    QmlProfilerDataState *dataState;
    QmlProfilerTraceTime *traceTime;

    QVector <double> partialCounts;
    QVector <int> partialCountWeights;
    int totalWeight;
    double progress;
    double previousProgress;
    qint64 estimatedTime;

    // file to load
    QString fileName;
};


QmlProfilerModelManager::QmlProfilerModelManager(Utils::FileInProjectFinder *finder, QObject *parent) :
    QObject(parent), d(new QmlProfilerModelManagerPrivate(this))
{
    d->totalWeight = 0;
    d->model = new QmlProfilerDataModel(finder, this);
    d->v8Model = new QV8ProfilerDataModel(finder, this);
    d->dataState = new QmlProfilerDataState(this, this);
    d->traceTime = new QmlProfilerTraceTime(this);
}

QmlProfilerModelManager::~QmlProfilerModelManager()
{
    delete d;
}

QmlProfilerTraceTime *QmlProfilerModelManager::traceTime() const
{
    return d->traceTime;
}

QmlProfilerDataModel *QmlProfilerModelManager::qmlModel() const
{
    return d->model;
}

QV8ProfilerDataModel *QmlProfilerModelManager::v8Model() const
{
    return d->v8Model;
}

bool QmlProfilerModelManager::isEmpty() const
{
    return d->model->isEmpty() && d->v8Model->isEmpty();
}

int QmlProfilerModelManager::count() const
{
    return d->model->count();
}

double QmlProfilerModelManager::progress() const
{
    return d->progress;
}

int QmlProfilerModelManager::registerModelProxy()
{
    d->partialCounts << 0;
    d->partialCountWeights << 1;
    d->totalWeight++;
    return d->partialCounts.count()-1;
}

void QmlProfilerModelManager::setProxyCountWeight(int proxyId, int weight)
{
    d->totalWeight += weight - d->partialCountWeights[proxyId];
    d->partialCountWeights[proxyId] = weight;
}

void QmlProfilerModelManager::modelProxyCountUpdated(int proxyId, qint64 count, qint64 max)
{
    d->progress -= d->partialCounts[proxyId] * d->partialCountWeights[proxyId] /
            d->totalWeight;

    if (max <= 0)
        d->partialCounts[proxyId] = 1;
    else
        d->partialCounts[proxyId] = (double)count / (double) max;

    d->progress += d->partialCounts[proxyId] * d->partialCountWeights[proxyId] /
            d->totalWeight;

    if (d->progress - d->previousProgress > 0.01) {
        d->previousProgress = d->progress;
        emit progressChanged();
    }
}

qint64 QmlProfilerModelManager::estimatedProfilingTime() const
{
    return d->estimatedTime;
}

void QmlProfilerModelManager::newTimeEstimation(qint64 estimation)
{
    d->estimatedTime = estimation;
}

void QmlProfilerModelManager::addQmlEvent(int type,
                                          int bindingType,
                                          qint64 startTime,
                                          qint64 length,
                                          const QStringList &data,
                                          const QmlDebug::QmlEventLocation &location,
                                          qint64 ndata1,
                                          qint64 ndata2,
                                          qint64 ndata3,
                                          qint64 ndata4,
                                          qint64 ndata5)
{
    // If trace start time was not explicitly set, use the first event
    if (d->traceTime->startTime() == -1)
        d->traceTime->setStartTime(startTime);

    QTC_ASSERT(state() == QmlProfilerDataState::AcquiringData, /**/);
    d->model->addQmlEvent(type, bindingType, startTime, length, data, location, ndata1, ndata2, ndata3, ndata4, ndata5);
}

void QmlProfilerModelManager::addV8Event(int depth, const QString &function, const QString &filename,
                                         int lineNumber, double totalTime, double selfTime)
{
    d->v8Model->addV8Event(depth, function, filename, lineNumber,totalTime, selfTime);
}

void QmlProfilerModelManager::complete()
{
    switch (state()) {
    case QmlProfilerDataState::ProcessingData:
        setState(QmlProfilerDataState::Done);
        emit dataAvailable();
        break;
    case QmlProfilerDataState::AcquiringData:
        // If trace end time was not explicitly set, use the last event
        if (d->traceTime->endTime() == 0)
            d->traceTime->setEndTime(d->model->lastTimeMark());
        setState(QmlProfilerDataState::ProcessingData);
        d->model->complete();
        d->v8Model->complete();
        break;
    case QmlProfilerDataState::Empty:
        setState(QmlProfilerDataState::Done);
        break;
    case QmlProfilerDataState::Done:
        break;
    default:
        emit error(tr("Unexpected complete signal in data model."));
        break;
    }
}

void QmlProfilerModelManager::modelProcessingDone()
{
    Q_ASSERT(state() == QmlProfilerDataState::ProcessingData);
    if (d->model->processingDone() && d->v8Model->processingDone())
        complete();
}

void QmlProfilerModelManager::save(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(tr("Could not open %1 for writing.").arg(filename));
        return;
    }

    QmlProfilerFileWriter writer;

    writer.setTraceTime(traceTime()->startTime(), traceTime()->endTime(), traceTime()->duration());
    writer.setV8DataModel(d->v8Model);
    writer.setQmlEvents(d->model->getEvents());
    writer.save(&file);
}

void QmlProfilerModelManager::load(const QString &filename)
{
    d->fileName = filename;
    load();
}

void QmlProfilerModelManager::setFilename(const QString &filename)
{
    d->fileName = filename;
}

void QmlProfilerModelManager::load()
{
    QString filename = d->fileName;

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(tr("Could not open %1 for reading.").arg(filename));
        return;
    }

    // erase current
    clear();

    setState(QmlProfilerDataState::AcquiringData);

    QmlProfilerFileReader reader;
    connect(&reader, SIGNAL(error(QString)), this, SIGNAL(error(QString)));
    connect(&reader, SIGNAL(rangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation,
                                        qint64, qint64, qint64, qint64, qint64)),
            this, SLOT(addQmlEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation,
                             qint64, qint64, qint64, qint64, qint64)));
    connect(&reader, SIGNAL(traceStartTime(qint64)), traceTime(), SLOT(setStartTime(qint64)));
    connect(&reader, SIGNAL(traceEndTime(qint64)), traceTime(), SLOT(setEndTime(qint64)));
    reader.setV8DataModel(d->v8Model);
    reader.load(&file);

    complete();
}


void QmlProfilerModelManager::setState(QmlProfilerDataState::State state)
{
    d->dataState->setState(state);
}

QmlProfilerDataState::State QmlProfilerModelManager::state() const
{
    return d->dataState->state();
}

void QmlProfilerModelManager::clear()
{
    setState(QmlProfilerDataState::ClearingData);
    for (int i = 0; i < d->partialCounts.count(); i++)
        d->partialCounts[i] = 0;
    d->progress = 0;
    d->previousProgress = 0;
    d->model->clear();
    d->v8Model->clear();
    d->traceTime->clear();

    setState(QmlProfilerDataState::Empty);
}

void QmlProfilerModelManager::prepareForWriting()
{
    setState(QmlProfilerDataState::AcquiringData);
}

} // namespace QmlProfiler
