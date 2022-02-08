/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "baseconnectionmanager.h"
#include "nodeinstanceserverinterface.h"

#include <QElapsedTimer>
#include <QFile>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ConnectionManager : public BaseConnectionManager
{
    Q_OBJECT

public:
    ConnectionManager();
    ~ConnectionManager() override;
    enum PuppetStreamType { FirstPuppetStream, SecondPuppetStream, ThirdPuppetStream };

    void setUp(NodeInstanceServerInterface *nodeInstanceServerProxy,
               const QString &qrcMappingString,
               ProjectExplorer::Target *target,
               AbstractView *view) override;
    void shutDown() override;

    void writeCommand(const QVariant &command) override;

    quint32 writeCounter() const override;

protected:
    using BaseConnectionManager::processFinished;
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &connectionName) override;
    std::vector<Connection> &connections() { return m_connections; }

    quint32 &writeCommandCounter() { return m_writeCommandCounter; }

private:
    void printProcessOutput(QProcess *process, const QString &connectionName);
    void closeSocketsAndKillProcesses();

private:
    std::vector<Connection> m_connections;
    quint32 m_writeCommandCounter = 0;
};

} // namespace QmlDesigner
