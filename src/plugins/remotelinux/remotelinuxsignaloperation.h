/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REMOTELINUXSIGNALOPERATION_H
#define REMOTELINUXSIGNALOPERATION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>

namespace QSsh {
class SshConnectionParameters;
class SshRemoteProcessRunner;
}

namespace RemoteLinux {

class REMOTELINUX_EXPORT RemoteLinuxSignalOperation
        : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    virtual ~RemoteLinuxSignalOperation();

    void killProcess(int pid);
    void killProcess(const QString &filePath);
    void interruptProcess(int pid);
    void interruptProcess(const QString &filePath);

protected:
    RemoteLinuxSignalOperation(const QSsh::SshConnectionParameters &sshParameters);

private slots:
    void runnerProcessFinished();
    void runnerConnectionError();

private:
    virtual QString killProcessByNameCommandLine(const QString &filePath) const;
    virtual QString interruptProcessByNameCommandLine(const QString &filePath) const;
    void run(const QString &command);
    void finish();

    const QSsh::SshConnectionParameters m_sshParameters;
    QSsh::SshRemoteProcessRunner *m_runner;

    friend class LinuxDevice;
};

}

#endif // REMOTELINUXSIGNALOPERATION_H
