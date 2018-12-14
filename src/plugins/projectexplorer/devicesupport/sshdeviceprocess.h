/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "deviceprocess.h"

#include <memory>

namespace ProjectExplorer {

class Runnable;

class PROJECTEXPLORER_EXPORT SshDeviceProcess : public DeviceProcess
{
    Q_OBJECT
public:
    explicit SshDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = nullptr);
    ~SshDeviceProcess() override;

    void start(const Runnable &runnable) override;
    void interrupt() override;
    void terminate() override;
    void kill() override;

    QProcess::ProcessState state() const override;
    QProcess::ExitStatus exitStatus() const override;
    int exitCode() const override;
    QString errorString() const override;

    QByteArray readAllStandardOutput() override;
    QByteArray readAllStandardError() override;

    qint64 write(const QByteArray &data) override;

private:
    void handleConnected();
    void handleConnectionError();
    void handleDisconnected();
    void handleProcessStarted();
    void handleProcessFinished(const QString &error);
    void handleStdout();
    void handleStderr();
    void handleKillOperationFinished(const QString &errorMessage);
    void handleKillOperationTimeout();

    virtual QString fullCommandLine(const Runnable &runnable) const;
    virtual qint64 processId() const;

    class SshDeviceProcessPrivate;
    friend class SshDeviceProcessPrivate;
    const std::unique_ptr<SshDeviceProcessPrivate> d;
};

} // namespace ProjectExplorer
