// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
               AbstractView *view,
               ExternalDependenciesInterface &externalDependencies) override;
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
