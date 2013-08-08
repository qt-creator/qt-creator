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

#include "qmlprofilerstatemanager.h"

#include <QDebug>
#include <utils/qtcassert.h>

// uncomment for printing the state changes to debug output
//#define _DEBUG_PROFILERSTATE_

namespace QmlProfiler {
namespace Internal {

inline QString stringForState(int state) {
    switch (state) {
    case QmlProfilerStateManager::Idle: return QLatin1String("Idle");
    case QmlProfilerStateManager::AppStarting: return QLatin1String("AppStarting");
    case QmlProfilerStateManager::AppRunning: return QLatin1String("AppRunning");
    case QmlProfilerStateManager::AppStopRequested: return QLatin1String("AppStopRequested");
    case QmlProfilerStateManager::AppReadyToStop: return QLatin1String("AppReadyToStop");
    case QmlProfilerStateManager::AppStopped: return QLatin1String("AppStopped");
    case QmlProfilerStateManager::AppDying: return QLatin1String("AppDying");
    case QmlProfilerStateManager::AppKilled: return QLatin1String("AppKilled");
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
    default: {
        const QString message = QString::fromLatin1("Switching to unknown state in %1:%2").arg(QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
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
