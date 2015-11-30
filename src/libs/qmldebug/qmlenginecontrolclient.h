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

#ifndef QMLENGINECONTROLCLIENT_H
#define QMLENGINECONTROLCLIENT_H

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

signals:
    void engineAboutToBeAdded(int engineId, const QString &name);
    void engineAdded(int engineId, const QString &name);
    void engineAboutToBeRemoved(int engineId, const QString &name);
    void engineRemoved(int engineId, const QString &name);

protected:
    void messageReceived(const QByteArray &);
    void sendCommand(CommandType command, int engineId);

    struct EngineState {
        EngineState(CommandType command = InvalidCommand) : releaseCommand(command), blockers(0) {}
        CommandType releaseCommand;
        int blockers;
    };

    QMap<int, EngineState> m_blockedEngines;
};

}

#endif // QMLENGINECONTROLCLIENT_H
