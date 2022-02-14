/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "remotelinux_export.h"

#include <utils/processinterface.h>

namespace RemoteLinux {

class LinuxDevice;
class SshProcessInterfacePrivate;

class REMOTELINUX_EXPORT SshProcessInterface : public Utils::ProcessInterface
{
    Q_OBJECT

public:
    SshProcessInterface(const LinuxDevice *linuxDevice);
    ~SshProcessInterface();

protected:
    void emitStarted(qint64 processId);
    // To be called from leaf destructor.
    // Can't call it from SshProcessInterface destructor as it calls virtual method.
    void killIfRunning();

private:
    virtual void handleStarted(qint64 processId);
    virtual void handleReadyReadStandardOutput(const QByteArray &outputData);

    virtual QString fullCommandLine(const Utils::CommandLine &commandLine) const = 0;
    virtual QString pidArgumentForKill() const;

    void start() final;
    qint64 write(const QByteArray &data) final;
    void sendControlSignal(Utils::ControlSignal controlSignal) final;

    bool waitForStarted(int msecs) final;
    bool waitForReadyRead(int msecs) final;
    bool waitForFinished(int msecs) final;

    friend class SshProcessInterfacePrivate;
    SshProcessInterfacePrivate *d = nullptr;
};

} // namespace RemoteLinux
