/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmlprofilerstatemanager.h"

#include <QDebug>
#include <utils/qtcassert.h>

// uncomment for printing the state changes to debug output
//#define _DEBUG_PROFILERSTATE_

namespace QmlProfiler {
namespace Internal {

inline QString stringForState(int state) {
    switch (state) {
    case QmlProfilerStateManager::Idle: return QString("Idle");
    case QmlProfilerStateManager::AppStarting: return QString("AppStarting");
    case QmlProfilerStateManager::AppRunning: return QString("AppRunning");
    case QmlProfilerStateManager::AppStopRequested: return QString("AppStopRequested");
    case QmlProfilerStateManager::AppReadyToStop: return QString("AppReadyToStop");
    case QmlProfilerStateManager::AppStopped: return QString("AppStopped");
    case QmlProfilerStateManager::AppDying: return QString("AppDying");
    case QmlProfilerStateManager::AppKilled: return QString("AppKilled");
    default: break;
    }
    return QString();
}

class QmlProfilerStateManager::QmlProfilerStateManagerPrivate
{
public:
    QmlProfilerStateManagerPrivate(QmlProfilerStateManager *qq) : q(qq) {}
    ~QmlProfilerStateManagerPrivate() {}

    QmlProfilerStateManager *q;

    QmlProfilerStateManager::QmlProfilerState m_currentState;
    bool m_clientRecording;
    bool m_serverRecording;
};
QmlProfilerStateManager::QmlProfilerStateManager(QObject *parent) :
    QObject(parent),d(new QmlProfilerStateManagerPrivate(this))
{
    d->m_currentState = Idle;
    d->m_clientRecording = true;
    d->m_serverRecording = false;
}

QmlProfilerStateManager::~QmlProfilerStateManager()
{
    delete d;
}

QmlProfilerStateManager::QmlProfilerState QmlProfilerStateManager::currentState()
{
    return d->m_currentState;
}

bool QmlProfilerStateManager::clientRecording()
{
    return d->m_clientRecording;
}

bool QmlProfilerStateManager::serverRecording()
{
    return d->m_serverRecording;
}

QString QmlProfilerStateManager::currentStateAsString()
{
    return stringForState(d->m_currentState);
}

void QmlProfilerStateManager::setCurrentState(QmlProfilerState newState)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Profiler state change request from" << stringForState(d->m_currentState) << "to" << stringForState(newState);
#endif
    QTC_ASSERT(d->m_currentState != newState, /**/);
    switch (newState) {
    case Idle:
        QTC_ASSERT(d->m_currentState == AppStarting ||
                   d->m_currentState == AppStopped ||
                   d->m_currentState == AppKilled,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppStarting:
        QTC_ASSERT(d->m_currentState == Idle,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppRunning:
        QTC_ASSERT(d->m_currentState == AppStarting,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppStopRequested:
        QTC_ASSERT(d->m_currentState == AppRunning,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppReadyToStop:
        QTC_ASSERT(d->m_currentState == AppStopRequested,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppStopped:
        QTC_ASSERT(d->m_currentState == AppReadyToStop ||
                   d->m_currentState == AppDying,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppDying:
        QTC_ASSERT(d->m_currentState == AppRunning,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppKilled:
        QTC_ASSERT(d->m_currentState == AppDying,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    default:
        qDebug() << tr("Switching to unknown state in %1:%2").arg(QString(__FILE__), QString::number(__LINE__));
        break;
    }

    d->m_currentState = newState;
    emit stateChanged();
}

void QmlProfilerStateManager::setClientRecording(bool recording)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Setting client recording flag from" << d->m_serverRecording << "to" << recording;
#endif
    if (d->m_clientRecording != recording) {
        d->m_clientRecording = recording;
        emit clientRecordingChanged();
    }
}

void QmlProfilerStateManager::setServerRecording(bool recording)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Setting server recording flag from" << d->m_serverRecording << "to" << recording;
#endif
    if (d->m_serverRecording != recording) {
        d->m_serverRecording = recording;
        emit serverRecordingChanged();
    }
}

}
}
