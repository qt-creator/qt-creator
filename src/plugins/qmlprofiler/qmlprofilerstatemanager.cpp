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

static QString stringForState(int state)
{
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

QmlProfilerStateManager::QmlProfilerStateManager(QObject *parent) :
    QObject(parent)
{
    m_currentState = Idle;
    m_clientRecording = true;
    m_serverRecording = false;
}

QmlProfilerStateManager::~QmlProfilerStateManager()
{
}

QmlProfilerStateManager::QmlProfilerState QmlProfilerStateManager::currentState() const
{
    return m_currentState;
}

bool QmlProfilerStateManager::clientRecording() const
{
    return m_clientRecording;
}

bool QmlProfilerStateManager::serverRecording() const
{
    return m_serverRecording;
}

QString QmlProfilerStateManager::currentStateAsString() const
{
    return stringForState(m_currentState);
}

void QmlProfilerStateManager::setCurrentState(QmlProfilerState newState)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Profiler state change request from" << currentStateAsString() << "to" << stringForState(newState);
#endif
    QTC_ASSERT(m_currentState != newState, /**/);
    switch (newState) {
    case Idle:
        QTC_ASSERT(m_currentState == AppStarting ||
                   m_currentState == AppStopped ||
                   m_currentState == AppKilled,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppStarting:
        QTC_ASSERT(m_currentState == Idle,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppRunning:
        QTC_ASSERT(m_currentState == AppStarting,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppStopRequested:
        QTC_ASSERT(m_currentState == AppRunning,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppReadyToStop:
        QTC_ASSERT(m_currentState == AppStopRequested,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppStopped:
        QTC_ASSERT(m_currentState == AppReadyToStop ||
                   m_currentState == AppDying,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppDying:
        QTC_ASSERT(m_currentState == AppRunning,
                   qDebug() << "from" << currentStateAsString());
        break;
    case AppKilled:
        QTC_ASSERT(m_currentState == AppDying,
                   qDebug() << "from" << currentStateAsString());
        break;
    default: {
        const QString message = QString::fromLatin1("Switching to unknown state in %1:%2").arg(QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
        break;
    }

    m_currentState = newState;
    emit stateChanged();
}

void QmlProfilerStateManager::setClientRecording(bool recording)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Setting client recording flag from" << m_serverRecording << "to" << recording;
#endif
    if (m_clientRecording != recording) {
        m_clientRecording = recording;
        emit clientRecordingChanged();
    }
}

void QmlProfilerStateManager::setServerRecording(bool recording)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Setting server recording flag from" << m_serverRecording << "to" << recording;
#endif
    if (m_serverRecording != recording) {
        m_serverRecording = recording;
        emit serverRecordingChanged();
    }
}

} // namespace Internal
} // namespace QmlProfiler
