// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebugclient.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlEngineControlClient : public QmlDebugClient
{
    Q_OBJECT
public:
    explicit QmlEngineControlClient(QmlDebugConnection *client);

    enum MessageType {
        EngineAboutToBeAdded,
        EngineAdded,
        EngineAboutToBeRemoved,
        EngineRemoved
    };

    enum CommandType {
        StartWaitingEngine,
        StopWaitingEngine,
        InvalidCommand
    };


    void blockEngine(int engineId);
    void releaseEngine(int engineId);

    QList<int> blockedEngines() const;

    void messageReceived(const QByteArray &) override;

signals:
    void engineAboutToBeAdded(int engineId, const QString &name);
    void engineAdded(int engineId, const QString &name);
    void engineAboutToBeRemoved(int engineId, const QString &name);
    void engineRemoved(int engineId, const QString &name);

protected:
    void sendCommand(CommandType command, int engineId);

    struct EngineState {
        EngineState(CommandType command = InvalidCommand) : releaseCommand(command), blockers(0) {}
        CommandType releaseCommand;
        int blockers;
    };

    QMap<int, EngineState> m_blockedEngines;
};

}
