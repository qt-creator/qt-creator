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

#ifndef QMLPROFILERSTATEMANAGER_H
#define QMLPROFILERSTATEMANAGER_H

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateManager : public QObject
{
    Q_OBJECT
public:
    enum QmlProfilerState {
        Idle,
        AppStarting,
        AppRunning,
        AppStopRequested,
        AppReadyToStop,
        AppStopped,
        AppDying,
        AppKilled
    };

    explicit QmlProfilerStateManager(QObject *parent = 0);
    ~QmlProfilerStateManager();

    QmlProfilerState currentState();
    bool clientRecording();
    bool serverRecording();

    QString currentStateAsString();

signals:
    void stateChanged();
    void clientRecordingChanged();
    void serverRecordingChanged();

public slots:
    void setCurrentState(QmlProfilerState newState);
    void setClientRecording(bool recording);
    void setServerRecording(bool recording);

private:
    class QmlProfilerStateManagerPrivate;
    QmlProfilerStateManagerPrivate *d;
};

}
}

#endif // QMLPROFILERSTATEMANAGER_H
