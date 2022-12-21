// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerstatemanager.h"

#include <QDebug>
#include <utils/qtcassert.h>

// uncomment for printing the state changes to debug output
//#define _DEBUG_PROFILERSTATE_

namespace QmlProfiler {

inline QString stringForState(int state) {
    switch (state) {
    case QmlProfilerStateManager::Idle: return QLatin1String("Idle");
    case QmlProfilerStateManager::AppRunning: return QLatin1String("AppRunning");
    case QmlProfilerStateManager::AppStopRequested: return QLatin1String("AppStopRequested");
    case QmlProfilerStateManager::AppDying: return QLatin1String("AppDying");
    default: break;
    }
    return QString();
}

class QmlProfilerStateManager::QmlProfilerStateManagerPrivate
{
public:
    QmlProfilerStateManagerPrivate(QmlProfilerStateManager *qq)
        : q(qq), m_currentState(Idle), m_clientRecording(true), m_serverRecording(false),
          m_requestedFeatures(0), m_recordedFeatures(0) {}

    ~QmlProfilerStateManagerPrivate() = default;

    QmlProfilerStateManager *q;

    QmlProfilerStateManager::QmlProfilerState m_currentState;
    bool m_clientRecording;
    bool m_serverRecording;
    quint64 m_requestedFeatures;
    quint64 m_recordedFeatures;
};
QmlProfilerStateManager::QmlProfilerStateManager(QObject *parent) :
    QObject(parent),d(new QmlProfilerStateManagerPrivate(this)) {}

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

quint64 QmlProfilerStateManager::requestedFeatures() const
{
    return d->m_requestedFeatures;
}

quint64 QmlProfilerStateManager::recordedFeatures() const
{
    return d->m_recordedFeatures;
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
        QTC_ASSERT(d->m_currentState == AppStopRequested ||
                   d->m_currentState == AppDying,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppRunning:
        QTC_ASSERT(d->m_currentState == Idle,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppStopRequested:
        QTC_ASSERT(d->m_currentState == AppRunning,
                   qDebug() << "from" << stringForState(d->m_currentState));
        break;
    case AppDying:
        QTC_ASSERT(d->m_currentState == AppRunning,
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
    qDebug() << "Setting client recording flag from" << d->m_clientRecording << "to" << recording;
#endif
    if (d->m_clientRecording != recording) {
        d->m_clientRecording = recording;
        emit clientRecordingChanged(recording);
    }
}

void QmlProfilerStateManager::setServerRecording(bool recording)
{
#ifdef _DEBUG_PROFILERSTATE_
    qDebug() << "Setting server recording flag from" << d->m_serverRecording << "to" << recording;
#endif
    if (d->m_serverRecording != recording) {
        d->m_serverRecording = recording;
        emit serverRecordingChanged(recording);
    }
}

void QmlProfilerStateManager::setRequestedFeatures(quint64 features)
{
    if (d->m_requestedFeatures != features) {
        d->m_requestedFeatures = features;
        emit requestedFeaturesChanged(features);
    }
}

void QmlProfilerStateManager::setRecordedFeatures(quint64 features)
{
    if (d->m_recordedFeatures != features) {
        d->m_recordedFeatures = features;
        emit recordedFeaturesChanged(features);
    }
}

} // namespace QmlProfiler
