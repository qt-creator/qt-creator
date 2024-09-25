// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "capturingconnectionmanager.h"

#include <coreplugin/messagebox.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QVariant>

namespace QmlDesigner {

void CapturingConnectionManager::setUp(NodeInstanceServerInterface *nodeInstanceServer,
                                       const QString &qrcMappingString,
                                       ProjectExplorer::Target *target,
                                       AbstractView *view,
                                       ExternalDependenciesInterface &externalDependencies)
{
    InteractiveConnectionManager::setUp(nodeInstanceServer,
                                        qrcMappingString,
                                        target,
                                        view,
                                        externalDependencies);

    int indexOfCapturePuppetStream = QCoreApplication::arguments().indexOf(
        "-capture-puppet-stream");
    if (indexOfCapturePuppetStream > 0) {
        const QString filePath = QCoreApplication::arguments().at(indexOfCapturePuppetStream + 1);
        m_captureFileForTest.setFileName(filePath);
        bool isOpen = m_captureFileForTest.open(QIODevice::WriteOnly);
        if (isOpen)
            qDebug() << "capture file is open:" << filePath;
        else
            qDebug() << "capture file could not be opened!";
    }
}

void CapturingConnectionManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &connectionName)
{
    if (m_captureFileForTest.isOpen()) {
        m_captureFileForTest.close();
        Core::AsynchronousMessageBox::warning(
            tr("QML Emulation Layer (QML Puppet - %1) Crashed").arg(connectionName),
            tr("You are recording a puppet stream and the emulations layer crashed. "
               "It is recommended to reopen the Qt Quick Designer and start again."));
    }

    InteractiveConnectionManager::processFinished(exitCode, exitStatus, connectionName);
}

void CapturingConnectionManager::writeCommand(const QVariant &command)
{
    InteractiveConnectionManager::writeCommand(command);

    if (m_captureFileForTest.isWritable()) {
        qDebug() << "command name: " << QMetaType::typeName(command.typeId());
        writeCommandToIODevice(command, &m_captureFileForTest, writeCommandCounter());
        qDebug() << "\tcatpure file offset: " << m_captureFileForTest.pos();
    }
}

} // namespace QmlDesigner
