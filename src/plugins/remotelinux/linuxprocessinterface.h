// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "sshprocessinterface.h"

namespace RemoteLinux {

class LinuxDevice;
class SshProcessInterfacePrivate;

class REMOTELINUX_EXPORT LinuxProcessInterface : public SshProcessInterface
{
public:
    LinuxProcessInterface(const LinuxDevice *linuxDevice);
    ~LinuxProcessInterface();

private:
    void handleSendControlSignal(Utils::ControlSignal controlSignal) override;

    void handleStarted(qint64 processId) final;
    void handleDone(const Utils::ProcessResultData &resultData) final;
    void handleReadyReadStandardOutput(const QByteArray &outputData) final;
    void handleReadyReadStandardError(const QByteArray &errorData) final;

    QString fullCommandLine(const Utils::CommandLine &commandLine) const final;

    QByteArray m_output;
    QByteArray m_error;
    bool m_pidParsed = false;
};

} // namespace RemoteLinux
