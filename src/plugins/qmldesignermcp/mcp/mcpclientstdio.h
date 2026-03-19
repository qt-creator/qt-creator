// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpclient.h"

#include <QPointer>
#include <QProcess>

namespace QmlDesigner {

/*!
 * \brief Stdio transport for McpClient (spawns the server as a QProcess).
 */
class McpClientStdio : public McpClient
{
    Q_OBJECT

public:
    explicit McpClientStdio(const QString &clientName, const QString &clientVersion, QObject *parent = nullptr);
    ~McpClientStdio() override;

    bool startProcess(
        const QString &command,
        const QStringList &args = {},
        const QString &workingDir = {},
        const QProcessEnvironment &env = QProcessEnvironment::systemEnvironment());

    bool isRunning() const override;
    void stop(int killTimeoutMs = 1500) override;

protected:
    void sendRpcToServer(const QJsonObject &obj) override;

private slots:
    void onStdoutReady();
    void onStderrReady();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError e);

private:
    QPointer<QProcess> m_process;
    QByteArray m_stdoutBuffer;
};

} // namespace QmlDesigner
